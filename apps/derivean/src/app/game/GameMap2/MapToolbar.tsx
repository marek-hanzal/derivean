/** @format */

import { BuildingIcon, CyclesInline } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { BackIcon, Button, LinkTo } from "@use-pico/client";
import type { FC } from "react";
import { CycleButton } from "~/app/game/CycleButton";

export namespace MapToolbar {
	export interface Props {
		userId: string;
		cycle: number;
	}
}

export const MapToolbar: FC<MapToolbar.Props> = ({ userId, cycle }) => {
	const { locale, mapId } = useParams({ from: "/$locale/map/$mapId" });

	return (
		<div
			className={
				"react-flow__panel flex flex-row gap-4 items-center p-2 border bg-white border-slate-300 shadow-xs"
			}
			onDoubleClick={(e) => {
				e.preventDefault();
				e.stopPropagation();
			}}
		>
			<div className={"flex flex-row items-center"}>
				<LinkTo
					to={"/$locale/game"}
					params={{ locale }}
				>
					<Button
						iconEnabled={BackIcon}
						variant={{ variant: "subtle" }}
					/>
				</LinkTo>
				<LinkTo
					to={"/$locale/map/$mapId/building/list"}
					params={{ locale, mapId }}
				>
					<Button
						iconEnabled={BuildingIcon}
						variant={{ variant: "subtle" }}
					/>
				</LinkTo>

				<CycleButton
					userId={userId}
					mapId={mapId}
				/>
			</div>

			<CyclesInline cycles={cycle} />
		</div>
	);
};
