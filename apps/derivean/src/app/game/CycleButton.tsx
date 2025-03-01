/** @format */

import { transaction } from "@derivean/db";
import { useMutation } from "@tanstack/react-query";
import { Button, useInvalidator } from "@use-pico/client";
import { useCallback, type FC } from "react";
import { withCycle } from "~/app/service/withCycle/withCycle";

export namespace CycleButton {
	export interface Props extends Button.Props {
		userId: string;
		mapId: string;
	}
}

export const CycleButton: FC<CycleButton.Props> = ({ userId, mapId, ...props }) => {
	const invalidator = useInvalidator([["Cycle"], ["GameMap"]]);

	const mutation = useMutation({
		mutationKey: ["useCycleMutation"],
		async mutationFn({ userId }: { userId: string }) {
			return transaction(async (tx) => {
				return withCycle({ tx, userId, mapId });
			});
		},
		async onSuccess() {
			await invalidator();
		},
	});

	return (
		<Button
			iconEnabled={"icon-[hugeicons--play]"}
			onClick={useCallback(() => {
				mutation.mutate({ userId });
			}, [mutation, userId])}
			loading={mutation.isPending}
			{...props}
		/>
	);
};
