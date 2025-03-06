/** @format */

import { BuildingIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { LinkTo } from "@use-pico/client";
import { tvc } from "@use-pico/common";
import type { FC } from "react";
import type { BuildingListPanel } from "~/app/game/GameMap2/Building/List/BuildingListPanel";

export namespace Item {
	export interface Props {
		building: BuildingListPanel.Building;
	}
}

export const Item: FC<Item.Props> = ({ building }) => {
	const { mapId, locale } = useParams({ from: "/$locale/map/$mapId" });

	return (
		<div
			className={tvc([
				"flex",
				"flex-col",
				"gap-2",
				"rounded-md",
				"border",
				"border-slate-300",
				"p-2",
				"cursor-default",
				"hover:bg-slate-100",
			])}
		>
			<div className={"flex flex-row items-center justify-between"}>
				<LinkTo
					icon={BuildingIcon}
					to={"/$locale/map/$mapId/building/$buildingId/view"}
					params={{ locale, mapId, buildingId: building.id }}
				>
					{building.name}
				</LinkTo>
			</div>
		</div>
	);
};
