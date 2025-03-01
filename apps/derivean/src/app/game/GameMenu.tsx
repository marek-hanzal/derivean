/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Menu, MenuLink, Tx } from "@use-pico/client";
import type { FC } from "react";

export namespace GameMenu {
	export interface Props extends Menu.Props {
		//
	}
}

export const GameMenu: FC<GameMenu.Props> = (props) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Menu {...props}>
			<MenuLink
				icon={BlueprintIcon}
				to={"/$locale/game/blueprint/list"}
				params={{ locale }}
			>
				<Tx label={"Blueprint list (menu)"} />
			</MenuLink>
		</Menu>
	);
};
