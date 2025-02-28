import { useParams } from "@tanstack/react-router";
import { Menu, MenuLink, Tx } from "@use-pico/client";
import type { Entity, IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { BlueprintIcon } from "~/app/icon/BlueprintIcon";
import { BuildingIcon } from "~/app/icon/BuildingIcon";
import { ProductionIcon } from "~/app/icon/ProductionIcon";
import { ResourceIcon } from "~/app/icon/ResourceIcon";

export namespace BlueprintIndexMenu {
	export interface Props extends Menu.Props, Entity.Schema<IdentitySchema> {
		//
	}
}

export const BlueprintIndexMenu: FC<BlueprintIndexMenu.Props> = ({ entity, ...props }) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Menu {...props}>
			<MenuLink
				icon={BlueprintIcon}
				to={"/$locale/game/blueprint/$id/view"}
				params={{ locale, id: entity.id }}>
				<Tx label={"View detail (menu)"} />
			</MenuLink>

			<MenuLink
				icon={BuildingIcon}
				to={"/$locale/game/blueprint/$id/dependencies"}
				params={{ locale, id: entity.id }}>
				<Tx label={"Blueprint dependencies (menu)"} />
			</MenuLink>

			<MenuLink
				icon={ResourceIcon}
				to={"/$locale/game/blueprint/$id/requirements"}
				params={{ locale, id: entity.id }}>
				<Tx label={"Required resources (menu)"} />
			</MenuLink>

			<MenuLink
				icon={ProductionIcon}
				to={"/$locale/game/blueprint/$id/production"}
				params={{ locale, id: entity.id }}>
				<Tx label={"Resource production (menu)"} />
			</MenuLink>

			<MenuLink
				icon={"icon-[solar--bomb-minimalistic-outline]"}
				to={"/$locale/game/blueprint/$id/conflicts"}
				params={{ locale, id: entity.id }}>
				<Tx label={"Blueprint conflicts (menu)"} />
			</MenuLink>
		</Menu>
	);
};
