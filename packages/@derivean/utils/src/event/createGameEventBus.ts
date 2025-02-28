/** @format */
import { EventBus } from "@use-pico/common";
import type { GameEventBus } from "./GameEventBus";

export const createGameEventBus = (): GameEventBus => {
	return EventBus<GameEventBus.Event>();
};
