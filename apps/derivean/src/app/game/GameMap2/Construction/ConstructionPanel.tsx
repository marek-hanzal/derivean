/** @format */

import { ConstructionIcon } from "@derivean/ui";
import { Tx } from "@use-pico/client";
import { tvc } from "@use-pico/common";
import type { FC } from "react";
import { Item } from "~/app/game/GameMap2/Construction/Item";
import { Panel } from "~/app/game/GameMap2/Panel";

export namespace ConstructionPanel {
	export interface Blueprint {
		id: string;
		name: string;
		count: number;
		cycles: number;
	}

	export interface Props extends Panel.PropsEx {
		userId: string;
		blueprints: Blueprint[];
	}
}

export const ConstructionPanel: FC<ConstructionPanel.Props> = ({
	userId,
	blueprints,
	...props
}) => {
	return (
		<Panel
			icon={ConstructionIcon}
			textTitle={<Tx label={"Construction (label)"} />}
			{...props}
		>
			{blueprints.length > 0 ? (
				blueprints.map((item) => {
					return (
						<Item
							key={item.id}
							blueprint={item}
							userId={userId}
						/>
					);
				})
			) : (
				<div
					className={tvc([
						"flex",
						"items-center",
						"justify-center",
						"rounded-sm",
						"border",
						"border-amber-400",
						"p-4",
						"bg-amber-200",
						"font-bold",
					])}
				>
					<Tx label={"There are no available blueprints (label)"} />
				</div>
			)}
		</Panel>
	);
};
