/** @format */

import { LandIcon } from "@derivean/ui";
import { Tx } from "@use-pico/client";
import type { FC } from "react";
import { Item } from "~/app/game/GameMap2/Land/Item";
import { Panel } from "~/app/game/GameMap2/Panel";

export namespace LandPanel {
	export interface Land {
		id: string;
		name: string;
		position: number;
	}

	export interface Props extends Panel.PropsEx {
		land: Land[];
	}
}

export const LandPanel: FC<LandPanel.Props> = ({ land, ...props }) => {
	return (
		<Panel
			icon={LandIcon}
			textTitle={<Tx label={"Land list (label)"} />}
			{...props}
		>
			{land.map((land) => (
				<Item
					key={land.id}
					land={land}
				/>
			))}
		</Panel>
	);
};
