/** @format */

import { ResourceIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { LinkTo, ListIcon, Preview, TitlePreview, Tx } from "@use-pico/client";
import type { IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace ResourcePreview {
	export interface Data extends IdentitySchema.Type {
		name: string;
	}

	export interface Props extends Preview.PropsEx<Data> {
		//
	}
}

export const ResourcePreview: FC<ResourcePreview.Props> = (props) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Preview
			title={({ entity }) => (
				<TitlePreview
					icon={ResourceIcon}
					title={entity.name}
				/>
			)}
			actions={() => (
				<>
					<LinkTo
						icon={ListIcon}
						to={"/$locale/root/resource/list"}
						params={{ locale }}
					>
						<Tx label={"Resource list (label)"} />
					</LinkTo>
				</>
			)}
			{...props}
		/>
	);
};
