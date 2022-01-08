//
// Created by abcz316 on 2020/2/26.
//

#ifndef REVERSE_SEARCH_ADDR_LINK_MAP_HELPER_H
#define REVERSE_SEARCH_ADDR_LINK_MAP_HELPER_H
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <assert.h>

struct baseOffsetInfo
{
	std::vector<std::weak_ptr<baseOffsetInfo>> vwpLastNode;
	uint64_t addr = 0;

#ifdef __linux__
	ssize_t offset;
#else
	SSIZE_T offset;
#endif
	std::vector<std::weak_ptr<baseOffsetInfo>> vwpNextNode;
};

static void PrintfAddrOffsetLinkMap(std::shared_ptr<baseOffsetInfo> startNode,
	std::function<void(const std::string & newSinglePath, size_t deepIndex/*start from zero*/)> OnShowNewSinglePath) {

	struct DeepStackIndexInfo {
		size_t nCurStackObjNextNodeIndexNum = 0; //当前栈对象里的对象NextNode纵向深度坐标
		std::weak_ptr<baseOffsetInfo> wpStackObj; //当前栈里的对象
		std::string strStackName; //当前栈的偏移路径全称
	} oDeepStackIndexInfo;
	std::vector<std::shared_ptr<DeepStackIndexInfo>> vspDeepStackIndexInfo; //路径栈


	std::shared_ptr<baseOffsetInfo> spCurItem = startNode;
	std::stringstream sstrTempShowAddrOffset;
	while (spCurItem) {

		sstrTempShowAddrOffset << " 0x" << std::hex << spCurItem->addr;
		if (spCurItem->offset < 0) {
			sstrTempShowAddrOffset << "-0x";
			sstrTempShowAddrOffset << std::hex << -spCurItem->offset << "]";
		}
		else {
			sstrTempShowAddrOffset << "+0x";
			sstrTempShowAddrOffset << std::hex << spCurItem->offset << "]";
		}

		OnShowNewSinglePath(sstrTempShowAddrOffset.str(), vspDeepStackIndexInfo.size());
	
		if (spCurItem->vwpNextNode.size()) {

			std::shared_ptr<DeepStackIndexInfo> spLastStack = std::make_shared<DeepStackIndexInfo>();
			spLastStack->nCurStackObjNextNodeIndexNum = 0;
			spLastStack->wpStackObj = spCurItem;
			spLastStack->strStackName = sstrTempShowAddrOffset.str();
			vspDeepStackIndexInfo.push_back(spLastStack);

			spCurItem = spCurItem->vwpNextNode[0].lock();
		}
		else {
			while (vspDeepStackIndexInfo.size()) {

				std::shared_ptr<DeepStackIndexInfo> spLastStack = vspDeepStackIndexInfo.back();
				spLastStack->nCurStackObjNextNodeIndexNum++;
				auto spLastItem = spLastStack->wpStackObj.lock();
				if (spLastItem && spLastItem->vwpNextNode.size() > spLastStack->nCurStackObjNextNodeIndexNum) {
					spCurItem = spLastItem->vwpNextNode[spLastStack->nCurStackObjNextNodeIndexNum].lock();

					sstrTempShowAddrOffset.clear();
					sstrTempShowAddrOffset.str("");
					sstrTempShowAddrOffset << spLastStack->strStackName;
					break;
				}
				else {
					vspDeepStackIndexInfo.pop_back();
				}
			}
			if (!vspDeepStackIndexInfo.size()) {
				spCurItem = nullptr;
			}

		}

	}
}
static void test_PrintfAddrOffsetLinkMap() {
	std::shared_ptr<baseOffsetInfo> demo1 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo21 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo22 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo23 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo31 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo32 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo33 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo34 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo41 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo42 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo43 = std::make_shared<baseOffsetInfo>();
	std::shared_ptr<baseOffsetInfo> demo44 = std::make_shared<baseOffsetInfo>();
	demo1->addr = 1;
	demo21->addr = 21;
	demo22->addr = 22;
	demo23->addr = 23;
	demo31->addr = 31;
	demo32->addr = 32;
	demo33->addr = 33;
	demo34->addr = 34;
	demo41->addr = 41;
	demo42->addr = 42;
	demo43->addr = 43;
	demo44->addr = 44;
	
	demo1->offset = 1;
	demo21->offset = 21;
	demo22->offset = 22;
	demo23->offset = 23;
	demo31->offset = 31;
	demo32->offset = 32;
	demo33->offset = 33;
	demo34->offset = 34;
	demo41->offset = 41;
	demo42->offset = 42;
	demo43->offset = 43;
	demo44->offset = 44;

	demo1->vwpNextNode.push_back(demo21);
	demo1->vwpNextNode.push_back(demo22);
	demo1->vwpNextNode.push_back(demo23);


	demo21->vwpNextNode.push_back(demo31);
	demo21->vwpNextNode.push_back(demo32);
	demo21->vwpNextNode.push_back(demo33);
	demo22->vwpNextNode.push_back(demo34);

	demo31->vwpNextNode.push_back(demo41);
	demo31->vwpNextNode.push_back(demo42);
	demo31->vwpNextNode.push_back(demo43);
	demo34->vwpNextNode.push_back(demo44);

	demo41->vwpLastNode.push_back(demo31);
	demo42->vwpLastNode.push_back(demo31);
	demo43->vwpLastNode.push_back(demo31);
	demo44->vwpLastNode.push_back(demo34);

	demo31->vwpLastNode.push_back(demo21);
	demo32->vwpLastNode.push_back(demo21);
	demo33->vwpLastNode.push_back(demo21);
	demo34->vwpLastNode.push_back(demo22);

	demo21->vwpLastNode.push_back(demo1);
	demo22->vwpLastNode.push_back(demo1);
	demo23->vwpLastNode.push_back(demo1);

	std::map<uint64_t, std::shared_ptr<baseOffsetInfo>> baseAddrOffsetRecordMap; //记录搜索到的地址偏移
	baseAddrOffsetRecordMap.insert({ 1, demo1 });

	baseAddrOffsetRecordMap.insert({ 21, demo21 });
	baseAddrOffsetRecordMap.insert({ 22, demo22 });
	baseAddrOffsetRecordMap.insert({ 23, demo23 });

	baseAddrOffsetRecordMap.insert({ 31, demo31 });
	baseAddrOffsetRecordMap.insert({ 32, demo32 });
	baseAddrOffsetRecordMap.insert({ 33, demo33 });
	baseAddrOffsetRecordMap.insert({ 34, demo33 });

	baseAddrOffsetRecordMap.insert({ 41, demo41 });
	baseAddrOffsetRecordMap.insert({ 42, demo42 });
	baseAddrOffsetRecordMap.insert({ 43, demo43 });
	baseAddrOffsetRecordMap.insert({ 44, demo44 });

	std::string result;
	PrintfAddrOffsetLinkMap(demo1, [&result](const std::string & newSinglePath, size_t deepIndex/*start from zero*/)->void {
		result += newSinglePath + "\n";
	});
	assert(result.length());
}
#endif //REVERSE_SEARCH_ADDR_LINK_MAP_HELPER_H
