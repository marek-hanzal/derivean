/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { LinkTo, ListIcon, Preview, TitlePreview, Tx } from "@use-pico/client";
import type { IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace BlueprintPreview {
	export interface Data extends IdentitySchema.Type {
		name: string;
	}

	export interface Props extends Preview.PropsEx<Data> {
		//
	}
}

export const BlueprintPreview: FC<BlueprintPreview.Props> = (props) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Preview
			title={({ entity }) => (
				<TitlePreview
					icon={BlueprintIcon}
					title={entity.name}
				/>
			)}
			actions={() => {
				return (
					<>
						<LinkTo
							icon={ListIcon}
							to={"/$locale/game/blueprint/list"}
							params={{ locale }}
						>
							<Tx label={"Blueprint list (label)"} />
						</LinkTo>
					</>
				);
			}}
			{...props}
		/>
	);
};
