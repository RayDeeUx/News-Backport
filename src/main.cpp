using namespace geode::prelude;

#include "NewsPopup.hpp"
#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		auto menu = this->getChildByID("more-games-menu");

		menu->getChildByID("more-games-button")->setVisible(false);

		auto newsSpr = CCSprite::createWithSpriteFrameName("newsBtn_001.png"_spr);

        newsSpr->setScale(1.35f);

		auto newsButton = CCMenuItemSpriteExtra::create(
			newsSpr,
			this,
			menu_selector(MyMenuLayer::onNews)
		);

		newsButton->setID("news-button");

		menu->addChild(newsButton);
		menu->updateLayout();

		return true;
	}

	void onNews(CCObject*) {
		NewsPopup::create()->show();
	}
};