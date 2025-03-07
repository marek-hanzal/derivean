/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { ListIcon, Preview, TitlePreview, Tx } from "@use-pico/client";
import type { IdentitySchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";

export namespace BlueprintPreview {
	export interface Data extends IdentitySchema.Type {
		name: string;
	}

	export interface Props extends Preview.PropsEx<Data> {
		linkList: FC<{
			icon: string;
			to: "/$locale/root/blueprint/list";
			params: { locale: string };
			children: ReactNode;
		}>;
		linkEditor: FC<{
			icon: string;
			to: "/$locale/root/editor";
			params: { locale: string };
			search: { zoomTo: string };
			children: ReactNode;
		}>;
	}
}

export const BlueprintPreview: FC<BlueprintPreview.Props> = ({
	linkList: LinkList,
	linkEditor: LinkEditor,
	...props
}) => {
	const { locale } = useParams({ from: "/$locale" });

	return (
		<Preview
			title={({ entity }) => (
				<TitlePreview
					icon={BlueprintIcon}
					title={entity.name}
				/>
			)}
			actions={({ entity }) => {
				return (
					<>
						<LinkList
							icon={ListIcon}
							to={"/$locale/root/blueprint/list"}
							params={{ locale }}
						>
							<Tx label={"Blueprint list (label)"} />
						</LinkList>

						<LinkEditor
							icon={"icon-[ph--graph-light]"}
							to={"/$locale/root/editor"}
							params={{ locale }}
							search={{ zoomTo: entity.id }}
						>
							<Tx label={"Editor (label)"} />
						</LinkEditor>
					</>
				);
			}}
			{...props}
		/>
	);
};
