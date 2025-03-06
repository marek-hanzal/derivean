/** @format */

import { ConstructionIcon, CyclesInline } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import { Badge, Button, LinkTo, useInvalidator } from "@use-pico/client";
import { type FC } from "react";
import type { ConstructionPanel } from "~/app/game/GameMap2/Construction/ConstructionPanel";
import { ItemCss } from "~/app/game/GameMap2/Construction/ItemCss";
import { withConstructionQueue } from "~/app/service/withConstructionQueue";

export namespace Item {
	export interface Props extends ItemCss.Props {
		blueprint: ConstructionPanel.Blueprint;
		userId: string;
	}
}

export const Item: FC<Item.Props> = ({ blueprint, userId, variant, tva = ItemCss, css }) => {
	const { mapId, locale } = useParams({ from: "/$locale/map/$mapId" });
	const invalidator = useInvalidator([["GameMap"]]);

	const constructionMutation = useMutation({
		async mutationFn({ blueprintId, plotId }: { blueprintId: string; plotId: string }) {
			return withConstructionQueue({ userId, blueprintId, plotId, plan: true, valid: false });
		},
		async onSuccess() {
			await invalidator();
		},
		onError(error) {
			console.error(error);
		},
	});
	const tv = tva({ ...variant, css }).slots;

	return (
		<div className={tv.base()}>
			<div className={"flex flex-row gap-2 items-center justify-between"}>
				<div className={"flex flex-row gap-2"}>
					<Badge>x{blueprint.count}</Badge>
					<LinkTo
						to={"/$locale/map/$mapId/blueprint/$blueprintId/requirements"}
						params={{ locale, mapId, blueprintId: blueprint.id }}
						css={{ base: ["font-bold"] }}
					>
						{blueprint.name}
					</LinkTo>
				</div>
				<div className={"flex flex-row gap-2"}>
					<CyclesInline cycles={blueprint.cycles} />
					<Button
						iconEnabled={ConstructionIcon}
						iconDisabled={ConstructionIcon}
						loading={constructionMutation.isPending}
						onClick={() => {
							constructionMutation.mutate({
								blueprintId: blueprint.id,
								plotId: "unknown`",
							});
						}}
					/>
				</div>
			</div>
		</div>
	);
};
