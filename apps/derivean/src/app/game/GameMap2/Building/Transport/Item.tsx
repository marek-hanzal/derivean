/** @format */

import { BuildingIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Badge, LinkTo } from "@use-pico/client";
import { tvc } from "@use-pico/common";
import type { FC } from "react";
import type { TransportPanel } from "~/app/game/GameMap2/Building/Transport/TransportPanel";

export namespace Item {
	export interface Props {
		transport: TransportPanel.Transport;
	}
}

export const Item: FC<Item.Props> = ({ transport }) => {
	const { mapId, locale } = useParams({ from: "/$locale/map/$mapId" });

	return (
		<div
			className={tvc([
				"flex",
				"flex-row",
				"gap-2",
				"items-center",
				"justify-between",
				"border",
				"p-4",
				"rounded-sm",
				"border-slate-200",
				"hover:border-slate-300",
				"hover:bg-slate-100",
			])}
		>
			<div className={"font-bold"}>{transport.resource}</div>
			<LinkTo
				icon={BuildingIcon}
				to={"/$locale/map/$mapId/building/$buildingId/view"}
				params={{ locale, mapId, buildingId: transport.sourceId }}
				search={{ zoomToId: transport.sourceId }}
			>
				{transport.source}
			</LinkTo>
			<Badge>x{transport.amount}</Badge>
		</div>
	);
};
