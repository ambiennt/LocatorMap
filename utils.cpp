#include "main.h"

void LocatorMapChestUtils::fillInAndSortOffsetsCache() {
	size_t i = 0;
	for (int8_t x = -MAX_DISPLACEMENT; x <= MAX_DISPLACEMENT; x++) {
		for (int8_t y = -MAX_DISPLACEMENT; y <= MAX_DISPLACEMENT; y++) {
			for (int8_t z = -MAX_DISPLACEMENT; z <= MAX_DISPLACEMENT; z++) {
				POS_OFFSETS_CACHE[i++] = {x, y, z};
			}
		}
	}
	std::sort(POS_OFFSETS_CACHE.begin(), POS_OFFSETS_CACHE.end());
}

bool LocatorMapChestUtils::isSafeBlock(const Block &block, bool isAboveBlock) {
	if (!block.mLegacyBlock) { return false; }
	auto& legacy = *block.mLegacyBlock.get();
	if (isAboveBlock) {
		if (legacy.isUnbreakableBlock()) { return false; }
	}
	return (legacy.isAirBlock() ||
			legacy.hasBlockProperty(BlockProperty::Liquid) ||
			legacy.hasBlockProperty(BlockProperty::TopSnow) ||
			(legacy.getMaterialType() == MaterialType::ReplaceablePlant));
}

bool LocatorMapChestUtils::isSafeRegion(const BlockSource &region, int32_t leadX, int32_t leadY, int32_t leadZ) {
	return (isSafeBlock(region.getBlock(leadX, leadY, leadZ), false) &&
			isSafeBlock(region.getBlock(leadX, leadY + 1, leadZ), true));
}

BlockPos LocatorMapChestUtils::tryGetSafeLocatorMapChestPos(const Player &player) {

	BlockPos leading = player.getBlockPosGrounded();
	auto& region = *player.mRegion;

	static constexpr dx feetLevel{0, 0, 0}, eyeLevel{0, 1, 0};

	for (const auto& offset : POS_OFFSETS_CACHE) {
		if (offset == feetLevel) continue;
		if (offset == eyeLevel) continue;

		BlockPos current{leading.x + offset.x, leading.y + offset.y, leading.z + offset.z};
		if (isSafeRegion(region, current.x, current.y, current.z)) {
			leading = current;
			break;
		}
	}

	switch (player.mDimensionId) {
		case DimensionID::Overworld: {
			int32_t lowerBounds = ((player.mLevel->getWorldGeneratorType() == GeneratorType::Flat) ? 1 : 5);
			leading.y = std::clamp(leading.y, lowerBounds, 255);
			break;
		}
		case DimensionID::Nether: {
			leading.y = std::clamp(leading.y, 5, 122);
			break;
		}
		case DimensionID::TheEnd: {
			leading.y = std::clamp(leading.y, 0, 255);
			break;
		}
		default: break;
	}

	return leading;
}