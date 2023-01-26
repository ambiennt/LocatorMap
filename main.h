#pragma once

#include <hook.h>
#include <base/base.h>
#include <base/log.h>
#include <mods/CommandSupport.h>
#include <Actor/Player.h>
#include <Level/Level.h>
#include <Level/Dimension.h>
#include <Packet/TextPacket.h>
#include <Block/VanillaBlocks.h>
#include <Item/ItemStack.h>
#include <Item/VanillaItems.h>
#include <Item/MapItem.h>
#include <Item/ItemInstance.h>
#include <Item/MapItemSavedData.h>
#include <Container/Container.h>
#include <Block/BlockSource.h>
#include <BlockActor/ChestBlockActor.h>

#include <cstdint>
#include <string>
#include <optional>
#include <array>

class LocatorMapCommand : public Command {
	CommandSelector<Player> m_Selector{};
	DimensionID m_DimensionID;
	int32_t m_OriginX, m_OriginZ;
	int32_t m_Scale;
	bool m_SetChestIfNeeded;

	ItemInstance getLocatorMapItem() const;
	void giveLocatorMapItem(Player &player, const ItemInstance &mapItemInstance) const;
	std::optional<BlockPos> setLocatorMapChest(const Player &player, ItemStack &mapStack) const;
public:

	LocatorMapCommand() : m_DimensionID(DimensionID::Overworld), m_OriginX(0),
		m_OriginZ(0), m_Scale(1), m_SetChestIfNeeded(true) {
		this->m_Selector.setIncludeDeadPlayers(true);
	}

	virtual void execute(const CommandOrigin &origin, CommandOutput &output) override;
	static void setup(CommandRegistry *registry);

	static constexpr inline int32_t ORIGIN_MIN_X = -1000000, ORIGIN_MAX_X = 1000000; // arbitrary
	static constexpr inline int32_t ORIGIN_MIN_Z = -1000000, ORIGIN_MAX_Z = 1000000; // arbitrary
};

namespace LocatorMapChestUtils {

	struct dx {
    	int8_t x, y, z;
    
    	inline constexpr int32_t magnitude() const {
        	return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
    	}
    	inline constexpr bool operator<(dx rhs) const {
        	return this->magnitude() < rhs.magnitude();
    	}
		inline constexpr bool operator==(dx rhs) const {
			return (this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z);
		}
	};

	constexpr inline int8_t MAX_DISPLACEMENT = 5;
	inline std::array<dx, ((2 * MAX_DISPLACEMENT) + 1) * ((2 * MAX_DISPLACEMENT) + 1) * ((2 * MAX_DISPLACEMENT) + 1)> POS_OFFSETS_CACHE{};

	/*constexpr*/ void fillInAndSortOffsetsCache(); // boo c++17
	bool isSafeBlock(const Block &block, bool isAboveBlock);
	bool isSafeRegion(const BlockSource &region, int32_t leadX, int32_t leadY, int32_t leadZ);
	BlockPos tryGetSafeLocatorMapChestPos(const Player &player);
} // namespace LocatorMapUtils

DEF_LOGGER("LocatorMap");