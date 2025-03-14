/** @format */

import { ResourceIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Menu, MenuLink, Tx } from "@use-pico/client";
import type { Entity, IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace ResourceIndexMenu {
	export interface Props extends Menu.Props, Entity.Schema<IdentitySchema> {
		//
	}
}

export const ResourceIndexMenu: FC<ResourceIndexMenu.Props> = ({ entity, ...props }) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Menu {...props}>
			<MenuLink
				icon={ResourceIcon}
				to={"/$locale/root/resource/$id/view"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"View detail (menu)"} />
			</MenuLink>
		</Menu>
	);
};
