#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

struct NewsBlock {
    int type = 0;
    std::string text;
    std::string filename;
    float width = 0.f;
    float height = 0.f;
    bool clickable = false;
    std::string url;
};

struct NewsItem {
    int id = 0;
    std::string date;
    std::string title;
    std::vector<NewsBlock> blocks;
};

class NewsPopup : public geode::Popup, public FLAlertLayerProtocol {
protected:
    std::vector<NewsItem> m_items;
    int m_index = 0;

    ScrollLayer* m_scroll = nullptr;
    Scrollbar* m_scrollbar = nullptr;
    NineSlice* m_listBG = nullptr;
    LoadingSpinner* m_spinner = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCMenuItemSpriteExtra* m_leftBtn = nullptr;
    CCMenuItemSpriteExtra* m_rightBtn = nullptr;

    async::TaskHolder<web::WebResponse> m_request;

    bool init() override;
    void fetchNews();
    void onNewsLoaded(std::string const& raw);
    void showItem(int index);
    void clearContent();
    void openLinkSafe(std::string const& url);
    void FLAlert_Clicked(FLAlertLayer* layer, bool btn2) override;
    void keyDown(enumKeyCodes key, double p1) override;

    static std::string b64Decode(std::string s);
    static std::vector<NewsItem> parseNews(std::string const& data);

public:
    static NewsPopup* create();
};