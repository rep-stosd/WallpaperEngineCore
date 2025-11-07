
#include "stdafx.hpp"
#include "CScene.hpp"
#include "TextureLoader.hpp"

#include "steam_api.h"
#include "deps/simdjson.h"
#include "imgui.h"

int _steamStatus = 0;
bool _gSteamInit = false;
bool _gSteamQueryComplete = false;

struct SteamUIQuery_t {
    EUGCQuery queryType;
    int page = 1;
    char search[1024];
} _steamUIQuery;

struct SteamUIDownloaded_t {
    uint64 sizeOnDisk = 0;
    char folderPath[1024];
    uint32 timestamp = 0;
    PublishedFileId_t id;
    Scene::Desc desc;
};

struct SteamUIState_t {
    int page = 0;
    int downloading = 0;
    int iconsize = 192;
} _steamUIState;

std::vector<SteamUGCDetails_t> details;
std::unordered_map<uint64_t, MTLTexture> details_previews;

std::unordered_map<uint64_t, bool> downloadingItems;
std::unordered_map<uint64_t, SteamUIDownloaded_t> downloadedItems;

void SteamUI_DownloadThumb(UGCHandle_t handle) ;

class CSteamUI_Handler {
public:
    void SteamUI_OnWorkshopListQueryComplete(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure) {
        if (bIOFailure || pCallback->m_eResult != k_EResultOK) {
            printf("UGC query failed!\n");
            _steamStatus = 2;
            return;
        }
        
        for (int i = 0; i < 50; i++) {
            SteamUGCDetails_t detail;
            SteamUGC()->GetQueryUGCResult(pCallback->m_handle, i, &detail);
            details.push_back(detail);
            SteamUI_DownloadThumb(detail.m_hPreviewFile);
        }
        
        
        SteamUGC()->ReleaseQueryUGCRequest(pCallback->m_handle);
        _gSteamQueryComplete = true;
        
        
       // delete this;
    }

    
    void SteamUI_OnThumbDownloadComplete(RemoteStorageDownloadUGCResult_t *pCallback, bool bIOFailure) {
        if (bIOFailure || pCallback->m_eResult != k_EResultOK) {
            printf("failed!\n");
            return;
        }
        
        char *buffer = (char*)malloc(pCallback->m_nSizeInBytes);
        int32 bytesRead = SteamRemoteStorage()->UGCRead(
            pCallback->m_hFile,
            buffer,
            pCallback->m_nSizeInBytes,
            0,
            k_EUGCRead_ContinueReadingUntilFinished
        );
        if (bytesRead) {
            FIMEMORY *stream = FreeImage_OpenMemory((BYTE*)buffer, pCallback->m_nSizeInBytes);
            FIBITMAP *bmp2 = FreeImage_LoadFromMemory(FreeImage_GetFileTypeFromMemory(stream, 0), stream);
            FIBITMAP *bmp = FreeImage_ConvertTo32Bits(bmp2);
            MTLTexture tex;
            tex.create(FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), MTL::PixelFormatBGRA8Unorm, MTL::TextureType2D, MTL::StorageModeManaged, MTL::TextureUsageShaderRead);
            tex.upload(4, FreeImage_GetBits(bmp));
            details_previews[pCallback->m_hFile] = tex;
            FreeImage_Unload(bmp);
            FreeImage_Unload(bmp2);
            FreeImage_CloseMemory(stream);
        }
        free(buffer);
     
      //  delete this;
    }
    
    
    CCallResult<CSteamUI_Handler, RemoteStorageDownloadUGCResult_t> _callResult;
    CCallResult<CSteamUI_Handler, SteamUGCQueryCompleted_t> _callResult2;
    
};

void SteamUI_ParseItemInfo(Scene::Desc& desc) {
    if (!std::filesystem::exists(desc.folderPath + "/project.json"))
        return;
    
    simdjson::dom::parser parser;
    simdjson::dom::element root = parser.load(desc.folderPath + "/project.json");
    
    std::string type = "scene";
    if (root["type"].is_string())
        type = root["type"].get_c_str().value();
    
    if (type == "scene" || type == "Scene") {
        desc.type = 0;
    }
    else if (type == "video") {
        desc.type = 1;
    }
    else {
        desc.type = -1;
        puts("unsupported type!");
    }
    
    if (root["file"].is_string())
        desc.file = root["file"].get_c_str().value();
    
    if (root["title"].is_string())
        desc.title = root["title"].get_c_str().value();
    
}

std::vector<CSteamUI_Handler*> _handlergcList;

class CSteamUI_Static_Handler {
private:
    STEAM_CALLBACK( CSteamUI_Static_Handler, SteamUI_OnItemDownloadComplete, DownloadItemResult_t );
} _gStaticSteamUIHandler;

void CSteamUI_Static_Handler::SteamUI_OnItemDownloadComplete(DownloadItemResult_t *pCallback) {
    
    if (pCallback->m_eResult != k_EResultOK) {
        printf("failed!\n");
        return;
    }
    if (pCallback->m_unAppID != 431960)
        return;
    
    puts("workshop item downloaded");
    
    _steamUIState.downloading = 0;
    
    SteamUIDownloaded_t details;

    bool ok = SteamUGC()->GetItemInstallInfo(
        pCallback->m_nPublishedFileId, // Workshop file ID
        &details.sizeOnDisk,
        details.folderPath,
        sizeof(details.folderPath),
        &details.timestamp
    );

    details.id = pCallback->m_nPublishedFileId;
    
    details.desc.folderPath = details.folderPath;
    SteamUI_ParseItemInfo(details.desc);
    
    if (ok) {
        printf("Item installed at: %s (size: %llu bytes)\n", details.folderPath, details.sizeOnDisk);
        downloadingItems[pCallback->m_nPublishedFileId] = false;
        downloadedItems[pCallback->m_nPublishedFileId]  = details;
    } else {
        printf("Failed to get item install info\n");
    }
    
}

void SteamUI_Init() {
    _steamStatus = !SteamAPI_Init();
    
    SteamErrMsg errMsg = { 0 };
    if ( SteamAPI_InitEx( &errMsg ) != k_ESteamAPIInitResult_OK )
    {
        puts( "SteamAPI_Init() failed: " );
        puts( errMsg );
        _steamStatus = 1;
        return;
    }

    _gSteamInit = true;
}


void SteamUI_DownloadThumb(UGCHandle_t handle) {
    CSteamUI_Handler* _steamUIHandler = new CSteamUI_Handler();

    SteamAPICall_t apiCall = SteamRemoteStorage()->UGCDownload(handle, 0);
    _steamUIHandler->_callResult.Set(apiCall, _steamUIHandler, &CSteamUI_Handler::SteamUI_OnThumbDownloadComplete);
    _handlergcList.push_back(_steamUIHandler);
}

void SteamUI_DownloadItem(PublishedFileId_t id) {
    if (downloadedItems[id].id || downloadingItems[id])
        return;
    
    SteamUGC()->SubscribeItem(id);
    bool success = SteamUGC()->DownloadItem(id, true);
    if (success) {
        downloadingItems[id] = true;
        _steamUIState.downloading = 1;
    }
}


void SteamUI_DrawCell(SteamUGCDetails_t& detail) {
    ImGui::BeginChild((uint64_t)&detail, ImVec2(_steamUIState.iconsize,_steamUIState.iconsize));
    if (details_previews[detail.m_hPreviewFile]._pTexture) {
        ImColor col(255,255,255);
        if ( downloadingItems[detail.m_nPublishedFileId] || downloadedItems[detail.m_nPublishedFileId].id )
            col = ImColor(120,120,120);
            
            
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        if ( ImGui::ImageButton(detail.m_rgchTitle, (ImTextureID)details_previews[detail.m_hPreviewFile]._pTexture, ImVec2(_steamUIState.iconsize,_steamUIState.iconsize), ImVec2(0, 1), ImVec2(1,0), col, col ) ) {
            SteamUI_DownloadItem(detail.m_nPublishedFileId);
        }
        ImGui::PopStyleVar();
        
        ImVec2 cursorPos = ImGui::GetCursorPos();
        
        ImGui::SetCursorPos(ImVec2(0,cursorPos.y/2));
        if (downloadingItems[detail.m_nPublishedFileId]) {
          //  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::TextColored(ImColor(0.5f, 0.5f, 1.f), "DOWNLOADING");
           // ImGui::PopFont();
        }
        
        ImGui::SetCursorPos(ImVec2((_steamUIState.iconsize/2)-ImGui::CalcTextSize(detail.m_rgchTitle).x/2,cursorPos.y-50));
        ImGui::Text("%s", detail.m_rgchTitle);
    }
    ImGui::EndChild();
}


void SteamUI_RefreshWorkshopList(EUGCQuery ranking = k_EUGCQuery_RankedByTrend, int page = 1,  char const *searchValue = nullptr, bool clean = true) {
    _gSteamQueryComplete = false;
    
    if (clean) {
        details = {};
        for (auto& i : details_previews)
            i.second.destroy();
        details_previews = {};
        
        for (auto* i : _handlergcList) {
            i->_callResult.Cancel();
            i->_callResult2.Cancel();
            delete i;
        }
        _handlergcList = {};
    }
    
    static UGCQueryHandle_t queryHandle;
    
    if (!_gSteamQueryComplete) {
        queryHandle = SteamUGC()->CreateQueryAllUGCRequest(ranking, k_EUGCMatchingUGCType_Items, 431960, 431960, page);
        if (searchValue)
            SteamUGC()->SetSearchText(queryHandle, searchValue);
        
        auto apiCall = SteamUGC()->SendQueryUGCRequest(queryHandle);
        
        CSteamUI_Handler* _steamUIHandler = new CSteamUI_Handler();
        
        _steamUIHandler->_callResult2.Set(apiCall, _steamUIHandler, &CSteamUI_Handler::SteamUI_OnWorkshopListQueryComplete);
        _handlergcList.push_back(_steamUIHandler);
    }
    
}


bool tahoe_button_factory(const char* text, ImVec2 size, int glass_index = 0, ImVec2 modifier = {0,0} ) ;

void SteamUI_WorkshopList() {
    if (_gSteamQueryComplete) {
        
        for (int i = 0 ; i < details_previews.size() ; i++) {
            ImGui::SetNextItemAllowOverlap();
            SteamUI_DrawCell(details[i]);
            if ( (i+1) % std::max( ((int)ImGui::GetWindowWidth() / _steamUIState.iconsize), 1) )
                ImGui::SameLine();
        }
            
    }

}


extern Scene scene;

void SteamUI_DrawCell_local(SteamUIDownloaded_t& detail) {
    ImGui::BeginChild(detail.id, ImVec2(_steamUIState.iconsize,_steamUIState.iconsize));
    auto tex = TextureLoader::createUncompressedTexture((std::string(detail.folderPath) + "/preview.jpg"), 0);
    if (!tex._pTexture)
        tex = TextureLoader::createUncompressedTexture((std::string(detail.folderPath) + "/preview.gif"), 0);
    if (tex._pTexture)
    {
        ImColor col(255,255,255);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        if ( ImGui::ImageButton(" ", (ImTextureID)tex._pTexture, ImVec2(_steamUIState.iconsize,_steamUIState.iconsize), ImVec2(0,0), ImVec2(1,1), col, col ) ) {
            scene.init(detail.desc);
        }
        ImGui::PopStyleVar();
        
        ImVec2 cursorPos = ImGui::GetCursorPos();
        
        ImGui::SetCursorPos(ImVec2((_steamUIState.iconsize/2)-ImGui::CalcTextSize("99999999").x/2,cursorPos.y-50));
        ImGui::Text("%s", detail.desc.title.data());
    }
    else
        ImGui::Text("Item missing/corrupted!\n%lld", detail.id);
    ImGui::EndChild();
}


void SteamUI_PopulateLocalList() {
    auto sz = SteamUGC()->GetNumSubscribedItems();
    PublishedFileId_t pVecPublishedFileId[sz];
    SteamUGC()->GetSubscribedItems(pVecPublishedFileId, sz);
    
    for (int i = 0; i < sz; i++) {
        auto id = pVecPublishedFileId[i];
        SteamUIDownloaded_t downloaded;
        SteamUGC()->GetItemInstallInfo(id, &downloaded.sizeOnDisk, downloaded.folderPath, 1024, &downloaded.timestamp);
        downloaded.id = id;
     //   if (!std::filesystem::exists((std::string(downloaded.folderPath) + "/scene.pkg")))
     //       break;
        downloaded.desc.folderPath = downloaded.folderPath;
        SteamUI_ParseItemInfo(downloaded.desc);
        
        downloadedItems[id] = downloaded;
    }
    
}


void SteamUI_LocalList() {
    int id = 0;
    ImGui::Text("%d", SteamUGC()->GetNumSubscribedItems());
    for (auto& i : downloadedItems) {
        if (i.second.id == 0) continue;
        
        SteamUI_DrawCell_local(i.second);
        
        if ( (id+1) % std::max( ((int)ImGui::GetWindowWidth() / _steamUIState.iconsize), 1) )
            ImGui::SameLine();
        id++;
    }
}

void SteamUI_Hotbar() {

    if (ImGui::BeginPopup("Filter popup")) {

        ImGui::EndPopup();
    }
    
    static char const * items[] = {
        "RankedByVote",
        "RankedByPublicationDate",
        "RankedByTrend",
        "RankedByNumTimesReported",
        "RankedByTotalVotesAsc",
        "RankedByVotesUp",
        "RankedByTotalUniqueSubscriptions",
        "RankedByLastUpdatedDate",
        "RankedByTextSearch",
        "NotYetRated"
    };
    
    static const char * current_item = items[2];
    
    if (ImGui::Button("o"))
        SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, _steamUIQuery.page, _steamUIQuery.search);
    ImGui::SameLine();
    
    if (ImGui::Button("<"))
        SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, --_steamUIQuery.page, _steamUIQuery.search);
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(40);
    if ( ImGui::InputInt("##Page", &_steamUIQuery.page, 0) )
        SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, _steamUIQuery.page, _steamUIQuery.search);
    ImGui::SameLine();


    if (ImGui::Button(">"))
        SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, ++_steamUIQuery.page, _steamUIQuery.search);
    ImGui::SameLine();
    
    
    
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##Sort order", current_item))
    {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            bool is_selected = (current_item == items[n]);
            if (ImGui::Selectable(items[n], is_selected))
            {
                current_item = items[n];
                switch (n) {
                    case 0: _steamUIQuery.queryType = k_EUGCQuery_RankedByVote; break;
                    case 1: _steamUIQuery.queryType = k_EUGCQuery_RankedByPublicationDate; break;
                    case 2: _steamUIQuery.queryType = k_EUGCQuery_RankedByTrend; break;
                    case 3: _steamUIQuery.queryType = k_EUGCQuery_RankedByNumTimesReported; break;
                    case 4: _steamUIQuery.queryType = k_EUGCQuery_RankedByTotalVotesAsc; break;
                    case 5: _steamUIQuery.queryType = k_EUGCQuery_RankedByVotesUp; break;
                    case 6: _steamUIQuery.queryType = k_EUGCQuery_RankedByTotalUniqueSubscriptions; break;
                    case 7: _steamUIQuery.queryType = k_EUGCQuery_RankedByLastUpdatedDate; break;
                    case 8: _steamUIQuery.queryType = k_EUGCQuery_RankedByTextSearch; break;
                    case 9: _steamUIQuery.queryType = k_EUGCQuery_NotYetRated; break;
                        
                }
                SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, _steamUIQuery.page, _steamUIQuery.search);
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();

    ImGui::SetNextItemWidth(200);
    if ( ImGui::InputTextWithHint("##Search", "Search", _steamUIQuery.search, 512) ) {
        SteamUI_RefreshWorkshopList(_steamUIQuery.queryType, _steamUIQuery.page, _steamUIQuery.search);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Preview >>"))
        ;
    
    ImGui::SameLine();
    
    if (ImGui::Button("Filter >>"))
        ImGui::OpenPopup("Filter popup");
    
    
    ImGui::SameLine();
    ImGui::Text(  _steamUIState.downloading > 0 ? "Downloading ..." : " ");
    //ImGui::SameLine();
    
    
        
}


// note: this was rendered in a separate context in the opengl version, consider readding it here
//  it is impossible to interact with things behind explorer/finder for win32/osx
void Wallpaper64SteamUI() {
    
    if (!_gSteamInit && _steamStatus == 0) {
        SteamUI_Init();
        if (_gSteamInit)
            SteamUI_PopulateLocalList();
        //scene.initForVideo("/Users/apple/Library/Application Support/Steam/steamapps/workshop/content/431960/3038952051/MEC music 4k.mp4");
    }
    
    if (_steamStatus == 0) {
        
        SteamAPI_RunCallbacks();
        
        ImGui::Begin("WallpaperUI", 0,  ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        if ( ImGui::Button("Local") )
            _steamUIState.page = 0;
        ImGui::SameLine();
        if ( ImGui::Button("Workshop") )
            _steamUIState.page = 1;
        
        ImGui::SameLine();
        
        if ( ImGui::Button("Unsubscribe all") ) {
            for (auto& i : downloadedItems) {
                SteamUGC()->UnsubscribeItem(i.first);
            }
        }
        
        
        SteamUI_Hotbar() ;
        
        
        ImGui::BeginChild("List");
        
        switch (_steamUIState.page) {
            case 0:
                SteamUI_LocalList();
                break;
            case 1:
                SteamUI_WorkshopList();
                break;
        }
        
        ImGui::EndChild();
        
        ImGui::End();
    }
}

