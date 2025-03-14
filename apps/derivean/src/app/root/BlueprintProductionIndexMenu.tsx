/** @format */

import { BlueprintIcon, RequirementIcon, ResourceIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Menu, MenuLink, Tx } from "@use-pico/client";
import type { Entity, IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace BlueprintProductionIndexMenu {
	export interface Props extends Menu.Props, Entity.Schema<IdentitySchema> {
		//
	}
}

export const BlueprintProductionIndexMenu: FC<BlueprintProductionIndexMenu.Props> = ({
	entity,
	...props
}) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Menu {...props}>
			<MenuLink
				icon={RequirementIcon}
				to={"/$locale/root/blueprint/production/$id/requirements"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"Production requirements (menu)"} />
			</MenuLink>

			<MenuLink
				icon={ResourceIcon}
				to={"/$locale/root/blueprint/production/$id/resources"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"Production resource requirements (menu)"} />
			</MenuLink>

			<MenuLink
				icon={BlueprintIcon}
				to={"/$locale/root/blueprint/production/$id/dependencies"}
				params={{ locale, id: entity.id }}
			>
				<Tx label={"Production resource dependencies (menu)"} />
			</MenuLink>
		</Menu>
	);
};
