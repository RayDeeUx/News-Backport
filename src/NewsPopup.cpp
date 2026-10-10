using namespace geode::prelude;

#include "NewsPopup.hpp"
#include <Geode/cocos/support/zip_support/ZipUtils.h>
#include <Geode/utils/base64.hpp>
std::string NewsPopup::b64Decode(std::string s) {
    auto res = utils::base64::decode(s, utils::base64::Base64Variant::Url);
    if (!res) {
        res = utils::base64::decode(s, utils::base64::Base64Variant::Normal);
    }
    if (!res) return "";
    auto const& bytes = res.unwrap();
    return std::string(bytes.begin(), bytes.end());
}

std::vector<NewsItem> NewsPopup::parseNews(std::string const& data) {
    std::vector<NewsItem> items;
    auto parts = utils::string::split(data, "|");
    if (parts.empty()) return items;

    for (size_t i = 1; i < parts.size(); ++i) {
        NewsItem item;
        auto fields = utils::string::split(parts[i], ";");
        for (auto const& field : fields) {
            auto colon = field.find(':');
            if (colon == std::string::npos) continue;
            auto key = field.substr(0, colon);
            auto val = field.substr(colon + 1);

            if (key == "1") {
                item.id = numFromString<int>(val).unwrapOr(0);
            } else if (key == "2") {
                item.date = b64Decode(val);
            } else if (key == "3") {
                item.title = b64Decode(val);
            } else if (key == "4") {
                auto blocks = utils::string::split(val, "~");
                for (auto const& blk : blocks) {
                    auto bits = utils::string::split(blk, ",");
                    if (bits.empty()) continue;
                    NewsBlock b;
                    b.type = numFromString<int>(bits[0]).unwrapOr(0);
                    if (b.type == 0 && bits.size() > 1) {
                        b.text = b64Decode(bits[1]);
                    } else if (b.type == 1 && bits.size() > 1) {
                        b.filename = bits[1];
                        if (bits.size() > 2) b.width = numFromString<float>(bits[2]).unwrapOr(0.f);
                        if (bits.size() > 3) b.height = numFromString<float>(bits[3]).unwrapOr(0.f);
                        if (bits.size() > 4) b.clickable = bits[4] == "1";
                        if (bits.size() > 5 && !bits[5].empty()) b.url = b64Decode(bits[5]);
                    } else if (b.type == 2 && bits.size() > 1) {
                        b.text = b64Decode(bits[1]);
                        if (bits.size() > 2) b.url = b64Decode(bits[2]);
                    }
                    item.blocks.push_back(std::move(b));
                }
            }
        }

        items.push_back(std::move(item));
    }

    return items;
}

static std::string stripTags(std::string s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        if (s[i] == '<' && i + 1 < s.size()) {
            auto end = s.find('>', i);
            if (end != std::string::npos) {
                i = end + 1;
                continue;
            }
        }
        if (s[i] == ':') {
            auto end = s.find(':', i + 1);
            if (end != std::string::npos && end - i < 24) {
                bool ok = true;
                for (size_t j = i + 1; j < end; ++j) {
                    if (!std::isalnum(static_cast<unsigned char>(s[j])) && s[j] != '_') {
                        ok = false;
                        break;
                    }
                }
                if (ok && end > i + 1) {
                    i = end + 1;
                    continue;
                }
            }
        }
        out.push_back(s[i++]);
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '\n')) out.pop_back();
    return out;
}

#include <Geode/binding/TextArea.hpp>
bool NewsPopup::init() {
    if (!Popup::init(420.f, 285.f, "GJ_square01.png"))
        return false;

    this->setID("NewsPopup");
    this->setTitle("News");

    m_mainLayer->setID("main-layer");
    m_closeBtn->getParent()->setID("main-menu");
    m_title->setScale(1.f);

    auto bottomLeftCorner = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");

    bottomLeftCorner->setID("bottom-left-corner");
    bottomLeftCorner->setZOrder(5);
    bottomLeftCorner->setAnchorPoint({0.f, 0.f});
    bottomLeftCorner->setPosition({0.f, 0.f});

    m_mainLayer->addChild(bottomLeftCorner);

    auto topLeftCorner = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");

    topLeftCorner->setID("top-left-corner");
    topLeftCorner->setFlipY(true);
    topLeftCorner->setZOrder(5);
    topLeftCorner->setAnchorPoint({0.f, 1.f});
    topLeftCorner->setPosition({0.f, m_size.height});

    m_mainLayer->addChild(topLeftCorner);

    auto bottomRightCorner = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");

    bottomRightCorner->setID("bottom-right-corner");
    bottomRightCorner->setFlipX(true);
    bottomRightCorner->setZOrder(5);
    bottomRightCorner->setAnchorPoint({1.f, 0.f});
    bottomRightCorner->setPosition({m_size.width, 0.f});

    m_mainLayer->addChild(bottomRightCorner);

    auto topRightCorner = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");

    topRightCorner->setID("top-right-corner");
    topRightCorner->setFlipX(true);
    topRightCorner->setFlipY(true);
    topRightCorner->setZOrder(5);
    topRightCorner->setAnchorPoint({1.f, 1.f});
    topRightCorner->setPosition({m_size.width, m_size.height});

    m_mainLayer->addChild(topRightCorner);

    m_listBG = NineSlice::create("square02_001.png");

    m_listBG->setID("list-bg");
    m_listBG->setOpacity(55);
    m_listBG->setContentSize({350.f + 8.f, 210.f + 8.f});
    
    m_mainLayer->addChildAtPosition(m_listBG, Anchor::Center, ccp(0.f, -12.f));

    m_scroll = ScrollLayer::create({350.f, 210.f});

    m_scroll->setID("news-scroll");
    m_scroll->setTouchEnabled(true);

    m_mainLayer->addChild(m_scroll);

    m_scroll->setPosition(m_listBG->getPosition() - ccp(350.f / 2.f, 210.f / 2.f));

    m_scrollbar = Scrollbar::create(m_scroll);
    m_scrollbar->setID("news-scrollbar");

    m_mainLayer->addChild(m_scrollbar);

    m_scrollbar->setPosition(m_scroll->getPosition() + ccp(350.f + 6.f, 210.f / 2.f));

    auto bgPos = m_listBG->getPosition();
    auto bgSize = m_listBG->getContentSize();

    m_pageLabel = CCLabelBMFont::create(" ", "goldFont.fnt");

    m_pageLabel->setID("page-label");
    m_pageLabel->setScale(0.4f);
    m_pageLabel->setAnchorPoint({1.f, 0.5f});
    m_pageLabel->setPosition({bgPos.x + bgSize.width / 2.f - 4.f, bgPos.y + bgSize.height / 2.f + 12.f});
    
    m_mainLayer->addChild(m_pageLabel);

    auto menu = CCMenu::create();

    menu->setID("nav-menu");
    menu->setPosition({0.f, 0.f});

    this->addChild(menu);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    float cx = winSize.width / 2.f;
    float cy = winSize.height / 2.f;

    auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    
    leftSpr->setID("left-arrow-spr");

    m_leftBtn = CCMenuItemExt::createSpriteExtra(leftSpr, [this](auto) { if (m_index > 0) this->showItem(m_index - 1); });
    m_leftBtn->setID("left-arrow-btn");
    m_leftBtn->setPosition({cx - m_size.width / 2.f - 22.f, cy});

    menu->addChild(m_leftBtn);

    auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");

    rightSpr->setID("right-arrow-spr");
    rightSpr->setFlipX(true);

    m_rightBtn = CCMenuItemExt::createSpriteExtra(rightSpr, [this](auto) { if (m_index + 1 < static_cast<int>(m_items.size())) this->showItem(m_index + 1); });
    m_rightBtn->setID("right-arrow-btn");
    m_rightBtn->setPosition({cx + m_size.width / 2.f + 22.f, cy});

    menu->addChild(m_rightBtn);

    m_leftBtn->setVisible(false);
    m_rightBtn->setVisible(false);

    m_spinner = LoadingSpinner::create(40.f);
    m_spinner->setID("loading-spinner");
    m_mainLayer->addChildAtPosition(m_spinner, Anchor::Center, ccp(0.f, -12.f));

    this->setKeypadEnabled(true);
    this->fetchNews();

    return true;
}

void NewsPopup::keyDown(enumKeyCodes key, double p1) {
    if (key == enumKeyCodes::KEY_Left || key == enumKeyCodes::KEY_A) {
        if (m_index > 0) this->showItem(m_index - 1);
        return;
    }

    if (key == enumKeyCodes::KEY_Right || key == enumKeyCodes::KEY_D) {
        if (m_index + 1 < static_cast<int>(m_items.size())) this->showItem(m_index + 1);
        return;
    }

    Popup::keyDown(key, p1);
}

void NewsPopup::openLinkSafe(std::string const& url) {
    if (url.empty()) return;

    auto alert = FLAlertLayer::create(
        this, "Hold Up!",
        fmt::format("Links are spooky! Are you sure you want to go to <cy>{}</c>?", url),
        "Cancel", "Yes", 360.f
    );
    
    alert->setUserObject(CCString::create(url));
    alert->show();
}

void NewsPopup::FLAlert_Clicked(FLAlertLayer* layer, bool btn2) {
    if (!btn2) return;
    if (auto href = typeinfo_cast<CCString*>(layer->getUserObject())) {
        web::openLinkInBrowser(href->getCString());
    }
}

void NewsPopup::fetchNews() {
    m_request.spawn(
        web::WebRequest().get("https://geometrydashfiles.b-cdn.net/news/news.dat"),
        [this](web::WebResponse res) {
            if (res.ok()) {
                auto body = res.string().unwrapOr("");

                auto b64 = utils::base64::decode(body, utils::base64::Base64Variant::Url);
                if (!b64) b64 = utils::base64::decode(body, utils::base64::Base64Variant::Normal);

                if (b64) {
                    auto& compressed = b64.unwrap();
                    unsigned char* out = nullptr;
                    int outLen = ZipUtils::ccInflateMemory(
                        const_cast<unsigned char*>(compressed.data()),
                        static_cast<unsigned int>(compressed.size()),
                        &out
                    );
                    if (outLen > 0 && out) {
                        std::string text(reinterpret_cast<char*>(out), outLen);
                        delete[] out;
                        this->onNewsLoaded(text);
                        return;
                    }
                    std::string plain(compressed.begin(), compressed.end());
                    if (plain.find('|') != std::string::npos) {
                        this->onNewsLoaded(plain);
                        return;
                    }
                }
                if (body.find('|') != std::string::npos) {
                    this->onNewsLoaded(body);
                    return;
                }
            }

            if (m_spinner) m_spinner->setVisible(false);

            auto err = CCLabelBMFont::create("Failed to load news", "bigFont.fnt");

            err->setID("error-label");
            err->setScale(0.4f);
            
            m_mainLayer->addChildAtPosition(err, Anchor::Center, ccp(0.f, -12.f));
        }
    );
}

void NewsPopup::onNewsLoaded(std::string const& raw) {
    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }

    m_items = parseNews(raw);
    if (m_items.empty()) {
        auto err = CCLabelBMFont::create("No news found", "bigFont.fnt");
        err->setID("error-label");
        err->setScale(0.4f);
        m_mainLayer->addChildAtPosition(err, Anchor::Center, ccp(0.f, -12.f));
        return;
    }

    m_index = 0;
    this->showItem(0);
}

void NewsPopup::clearContent() {
    if (m_scroll && m_scroll->m_contentLayer) {
        m_scroll->m_contentLayer->removeAllChildren();
    }
}

void NewsPopup::showItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    m_index = index;
    auto const& item = m_items[index];

    this->clearContent();

    m_pageLabel->setString(fmt::format("{} OF {}", index + 1, m_items.size()).c_str());
    m_leftBtn->setVisible(index > 0);
    m_rightBtn->setVisible(index < static_cast<int>(m_items.size()) - 1);

    auto* content = m_scroll->m_contentLayer;
    content->removeAllChildren();
    content->setLayout(nullptr);

    float maxW = 350.f - 16.f;
    float padX = 350.f / 2.f;
    float y = 0.f;
    constexpr float gap = 12.f;
    constexpr float leftPad = 10.f;

    auto pushCenter = [&](CCNode* node, float height) {
        node->setAnchorPoint({0.5f, 1.f});
        node->setPosition({padX, -y});

        content->addChild(node);

        y += height + gap;
    };

    auto pushLeft = [&](CCNode* node, float height) {
        node->setAnchorPoint({0.f, 1.f});
        node->setPosition({leftPad, -y});

        content->addChild(node);

        y += height + gap;
    };

    auto dateLabel = CCLabelBMFont::create(item.date.c_str(), "goldFont.fnt");

    dateLabel->setID("date-label");
    dateLabel->setScale(0.42f);

    pushLeft(dateLabel, dateLabel->getScaledContentSize().height);

    if (item.title.find(":eyes:") != std::string::npos) {
        auto eyes = CCSprite::createWithSpriteFrameName("eyesEmoji_001.png"_spr);

        eyes->setID("title-eyes");

        auto ref = CCLabelBMFont::create("O", "bigFont.fnt");

        ref->setScale(0.5f);

        float targetH = ref->getScaledContentSize().height;
        auto rest = stripTags(item.title);

        if (rest.empty()) targetH *= 2.f;
        if (eyes->getContentSize().height > 0.f) {
            eyes->setScale(targetH / eyes->getContentSize().height);
        }

        pushLeft(eyes, eyes->getScaledContentSize().height);
    } else {
        auto titleStr = stripTags(item.title);
        auto titleLabel = CCLabelBMFont::create(titleStr.c_str(), "bigFont.fnt");

        titleLabel->setID("title-label");
        titleLabel->limitLabelWidth(maxW, 0.5f, 0.25f);

        pushLeft(titleLabel, titleLabel->getScaledContentSize().height);
    }

    int blockIdx = 0;
    for (auto const& block : item.blocks) {
        if (block.type == 0) {
            auto textStr = stripTags(block.text);
            if (textStr.empty()) continue;

            constexpr float textPad = 12.f;
            float textW = 350.f - textPad - 6.f;
            auto area = TextArea::create(textStr, "chatFont.fnt", 0.7f, textW, {0.f, 1.f}, 14.f, false);
            if (!area) continue;
            area->setID(fmt::format("text-block-{}", blockIdx).c_str());
            float textH = area->getContentSize().height;
            if (textH < 1.f) textH = 20.f;
            area->setAnchorPoint({0.f, 1.f});
            area->setPosition({textPad, -y});
            content->addChild(area);
            y += textH + 4.f + gap;
        } else if (block.type == 1) {
            constexpr float FIXED_IMG_W = 320.f;
            float aspect = (block.width > 0 && block.height > 0) ? (block.height / block.width) : (454.f / 800.f);
            float dispW = FIXED_IMG_W;
            float dispH = dispW * aspect;

            auto wrapper = CCNode::create();

            wrapper->setID(fmt::format("image-block-{}", blockIdx).c_str());
            wrapper->setContentSize({dispW, dispH});

            auto spr = LazySprite::create({dispW, dispH}, true);

            spr->setID(fmt::format("image-sprite-{}", blockIdx).c_str());
            spr->setAnchorPoint({0.5f, 0.5f});
            spr->setPosition({dispW / 2.f, dispH / 2.f});
            spr->setAutoResize(true);
            spr->loadFromUrl(std::string("https://geometrydashfiles.b-cdn.net/news/") + block.filename);

            auto addYtIcon = [&](CCNode* parent) {
                auto urlLower = block.url;
                for (auto& c : urlLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                bool isVideo = block.clickable && (
                    urlLower.find("youtu") != std::string::npos ||
                    urlLower.find("watch") != std::string::npos
                );

                if (!isVideo && block.clickable && !block.url.empty()) {
                    isVideo = true;
                }

                if (!isVideo) return;
                auto icon = CCSprite::createWithSpriteFrameName("youtubePlayerIcon_001.png"_spr);

                icon->setID(fmt::format("yt-icon-{}", blockIdx).c_str());
                icon->setScale(0.425f);
                icon->setPosition({dispW / 2.f, dispH / 2.f});
                icon->setZOrder(10);

                parent->addChild(icon);
            };

            if (block.clickable && !block.url.empty()) {
                auto url = block.url;
                auto linkMenu = CCMenu::create();

                linkMenu->setID(fmt::format("image-menu-{}", blockIdx).c_str());
                linkMenu->setContentSize({dispW, dispH});
                linkMenu->setAnchorPoint({0.f, 0.f});
                linkMenu->ignoreAnchorPointForPosition(false);
                linkMenu->setPosition({0.f, 0.f});

                wrapper->addChild(linkMenu);

                auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, url](auto) { this->openLinkSafe(url); });

                btn->setID(fmt::format("image-btn-{}", blockIdx).c_str());
                btn->m_animationEnabled = false;
                btn->m_colorEnabled = true;
                btn->setPosition({dispW / 2.f, dispH / 2.f});

                linkMenu->addChild(btn);

                addYtIcon(wrapper);
            } else {
                wrapper->addChild(spr);
            }

            pushCenter(wrapper, dispH);
        } else if (block.type == 2) {
            auto label = CCLabelBMFont::create(stripTags(block.text).c_str(), "goldFont.fnt");

            label->setID(fmt::format("btn-label-{}", blockIdx).c_str());
            label->setScale(0.48f);
            label->limitLabelWidth(maxW, 0.48f, 0.3f);

            float h = label->getScaledContentSize().height + 4.f;
            auto url = block.url;
            auto linkMenu = CCMenu::create();

            linkMenu->setID(fmt::format("btn-menu-{}", blockIdx).c_str());
            linkMenu->setContentSize({maxW, h});
            linkMenu->setAnchorPoint({0.5f, 0.5f});
            linkMenu->ignoreAnchorPointForPosition(false);

            auto btn = CCMenuItemExt::createSpriteExtra(label, [this, url](auto) { this->openLinkSafe(url); });

            btn->setID(fmt::format("btn-{}", blockIdx).c_str());
            btn->setPosition({maxW / 2.f, h / 2.f});

            linkMenu->addChild(btn);

            pushCenter(linkMenu, h);
        }

        blockIdx++;
    }

    float totalH = std::max(y, 210.f);
    content->setContentSize({350.f, totalH});

    for (auto child : CCArrayExt<CCNode*>(content->getChildren())) {
        child->setPositionY(child->getPositionY() + totalH);
    }

    m_scroll->scrollToTop();
}

NewsPopup* NewsPopup::create() {
    auto ret = new NewsPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}