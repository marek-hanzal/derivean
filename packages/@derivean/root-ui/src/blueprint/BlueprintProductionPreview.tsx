/** @format */

import { BlueprintIcon, ProductionIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { Preview, TitlePreview, Tx } from "@use-pico/client";
import type { IdentitySchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";

export namespace BlueprintProductionPreview {
	export interface Data extends IdentitySchema.Type {
		resource: string;
		blueprint: string;
		blueprintId: string;
	}

	export interface Props extends Preview.PropsEx<Data> {
		linkProduction: FC<{
			icon: string;
			to: "/$locale/root/blueprint/$id/production";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
	}
}

export const BlueprintProductionPreview: FC<BlueprintProductionPreview.Props> = ({
	linkProduction: LinkProduction,
	...props
}) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Preview
			title={({ entity }) => (
				<TitlePreview
					icon={ProductionIcon}
					title={entity.resource}
					subtitle={entity.blueprint}
				/>
			)}
			actions={({ entity }) => (
				<>
					<LinkProduction
						icon={BlueprintIcon}
						to={"/$locale/root/blueprint/$id/production"}
						params={{ locale, id: entity.blueprintId }}
					>
						<Tx label={"Building base detail (label)"} />
					</LinkProduction>
				</>
			)}
			{...props}
		/>
	);
};
