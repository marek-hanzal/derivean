import type { GameConfig } from "@derivean/utils";
import { tvc } from "@use-pico/common";
import { FC } from "react";
import type { GameEventBus } from "~/app/createGameEventBus";
import { Loop } from "~/app/map/Loop";
import { MapCanvas } from "~/app/map/MapCanvas";

export namespace Map {
	export interface Props {
		mapId: string;
		gameConfig: GameConfig;
		gameEventBus: GameEventBus;
		zoom?: number;
	}
}

export const Map: FC<Map.Props> = ({ mapId, gameConfig, gameEventBus, zoom = 8 }) => {
	console.log("Map re-render");

	return (
		<div className={tvc(["w-screen", "h-screen", "overflow-hidden"])}>
			<MapCanvas zoom={zoom}>
				<Loop
					mapId={mapId}
					gameConfig={gameConfig}
					gameEventBus={gameEventBus}
					zoom={zoom}
				/>
			</MapCanvas>
		</div>
	);
};
