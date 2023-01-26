#include "main.h"
#include <dllentry.h>

void dllenter() {}
void dllexit() {}
void PreInit() {
	LocatorMapChestUtils::fillInAndSortOffsetsCache();
	Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), &LocatorMapCommand::setup);
}
void PostInit() {}

std::optional<BlockPos> LocatorMapCommand::setLocatorMapChest(const Player &player, ItemStack &mapStack) const {

	// can't do anything, region is invalid at this point in time
	if (!player.isPlayerInitialized()) { return {}; }

	auto leadingChestPos = LocatorMapChestUtils::tryGetSafeLocatorMapChestPos(player);
	auto& region = *player.mRegion;
	region.setBlock(leadingChestPos, *VanillaBlocks::mChest, 3, nullptr);

	auto chestBlock = region.getBlockEntity(leadingChestPos);
	auto chestContainer = reinterpret_cast<FillingContainer*>(chestBlock->getContainer());

	chestContainer->addItem(mapStack);
	chestBlock->setCustomName("Locator Map Chest");
	reinterpret_cast<ChestBlockActor*>(chestBlock)->mNotifyPlayersOnChange = true;
	chestBlock->onChanged(region);

	return leadingChestPos;
}

// useful xrefs:
// ExplorationMapFunction::apply
// MapItem::_makeNewExplorationMap
// MapItem::inventoryTick
// MapItem::update
ItemInstance LocatorMapCommand::getLocatorMapItem() const {

	auto& lvl = *LocateService<Level>();
	ItemInstance mapItemInstance(*VanillaItems::mFilledMap, 1, 0);

	auto& saveData = lvl.createMapSavedData(ActorUniqueID::INVALID_ID,
		{this->m_OriginX, 0, this->m_OriginZ}, {static_cast<int32_t>(this->m_DimensionID)}, this->m_Scale);

	EmptyMapItem::addPlayerMarker(mapItemInstance);
	MapItem::setItemInstanceInfo(mapItemInstance, saveData);

	saveData.enableUnlimitedTracking();
	saveData.mPreviewIncomplete = true;

	if (lvl.getWorldGeneratorType() != GeneratorType::Flat) {
		auto dim = lvl.getDimension({static_cast<int32_t>(this->m_DimensionID)});

		// this will be nullptr if dimension hasn't been traveled to/loaded at all yet
		if (dim) {
			MapItem::renderBiomePreviewMap(*dim, saveData);
		}
	}

	return mapItemInstance;
}

void LocatorMapCommand::giveLocatorMapItem(Player &player, const ItemInstance &mapItemInstance) const {

	ItemStack mapStack(mapItemInstance);
	mapStack.setCustomName("§bUHC Locator Map");

	std::string giveMsg{};
	bool shouldSendInventory = false;

	if (player.getOffhandSlot().isNull()) {
		player.setOffhandSlot(mapStack);
		giveMsg = "§bA locator map has been added to your offhand!";
		shouldSendInventory = true;
	}
	else if (player.add(mapStack)) {
		giveMsg = "§bA locator map has been added to your inventory!";
		shouldSendInventory = true;
	}
	else {
		if (this->m_SetChestIfNeeded) {
			auto optChestPos = this->setLocatorMapChest(player, mapStack);
			if (optChestPos) {
				auto pos = *optChestPos;
				giveMsg = "§bA locator map has been set in a chest at " + std::to_string(pos.x) +
					", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + "!";
			}
		}
		else {
			giveMsg = "§cSomething went wrong while trying to give you a locator map.";
		}
	}

	if (shouldSendInventory) {
		player.sendInventory(false);
	}

	auto pkt = TextPacket::createTextPacket<TextPacketType::SystemMessage>(giveMsg);
	player.sendNetworkPacket(pkt);
}

void LocatorMapCommand::execute(const CommandOrigin &origin, CommandOutput &output) {

	auto selectedEntities = this->m_Selector.results(origin);
	if (selectedEntities.empty()) {
		output.error("No targets matched selector");
		return;
	}

	if (this->m_Scale <= 0) {
		output.error("Locator map scale must be greater than 0");
		return;
	}

	if (this->m_Scale > MapConstants::MAX_SCALE) {
		output.error("Locator map scale must not be greater than " + std::to_string(MapConstants::MAX_SCALE));
		return;
	}

	if ((this->m_OriginX < ORIGIN_MIN_X) ||
		(this->m_OriginX > ORIGIN_MAX_X) ||
		(this->m_OriginZ < ORIGIN_MIN_Z) ||
		(this->m_OriginZ > ORIGIN_MAX_Z)) {

		output.error("Locator map origin must be within the range of (" +
			std::to_string(ORIGIN_MIN_X) + ", " +  std::to_string(ORIGIN_MAX_X) + ") - (" +
			std::to_string(ORIGIN_MIN_Z) + ", " +  std::to_string(ORIGIN_MAX_Z) + ")");
		return;
	}

	auto mapItemInstance = this->getLocatorMapItem();
	for (auto& player : selectedEntities) {
		this->giveLocatorMapItem(*player, mapItemInstance);
	}

	int32_t selectorCount = selectedEntities.count();
	output.success("Successfully given locator map to " + std::to_string(selectorCount) + ((selectorCount == 1) ? " player" :  " players"));
}

void LocatorMapCommand::setup(CommandRegistry *registry) {
	using namespace commands;

	registry->registerCommand("locatormap", "Gives a filled-in locator map.",
		CommandPermissionLevel::GameMasters, CommandFlagUsage, CommandFlagNone);

	addEnum<DimensionID>(registry, "dimensionType", {
		{ "overworld", DimensionID::Overworld },
		{ "nether", DimensionID::Nether },
		{ "end", DimensionID::TheEnd }
	});

	registry->registerOverload<LocatorMapCommand>("locatormap",
		mandatory(&LocatorMapCommand::m_Selector, "player"),
		mandatory<CommandParameterDataType::ENUM>(&LocatorMapCommand::m_DimensionID, "dimension", "dimensionType"),
		optional(&LocatorMapCommand::m_OriginX, "originX"),
		optional(&LocatorMapCommand::m_OriginZ, "originZ"),
		optional(&LocatorMapCommand::m_Scale, "scale"),
		optional(&LocatorMapCommand::m_SetChestIfNeeded, "setChestIfNeeded")
	);
}