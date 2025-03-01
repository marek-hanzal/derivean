/** @format */

import { BuildingIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Menu, MenuLink, Tx, UserIcon } from "@use-pico/client";
import type { Entity, IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace UserIndexMenu {
	export interface Props extends Menu.Props, Entity.Schema<IdentitySchema> {
		//
	}
}

export const UserIndexMenu: FC<UserIndexMenu.Props> = ({ entity, ...props }) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Menu {...props}>
			<MenuLink
				icon={UserIcon}
				to={"/$locale/root/user/$id/view"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"View detail (menu)"} />
			</MenuLink>

			<MenuLink
				icon={BuildingIcon}
				to={"/$locale/root/user/$id/building/list"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"Building list (menu)"} />
			</MenuLink>
		</Menu>
	);
};
