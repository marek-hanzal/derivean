/** @format */

import { transaction } from "@derivean/db";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionClick,
	ActionMenu,
	Table,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
} from "@use-pico/client";
import type { IdentitySchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";

export namespace UserTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		login: string;
	}

	export interface Context {
		linkView: FC<{
			to: "/$locale/root/user/$id/view";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
	}
}

const column = withColumn<UserTable.Data, UserTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"User name (label)"} />;
		},
		render({ data, value, context: { linkView: LinkView } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<LinkView
					to={"/$locale/root/user/$id/view"}
					params={{ locale, id: data.id }}
				>
					{value}
				</LinkView>
			);
		},
		size: 14,
	}),
	column({
		name: "login",
		header() {
			return <Tx label={"User login (label)"} />;
		},
		render({ value }) {
			return value;
		},
		size: 14,
	}),
];

export namespace UserTable {
	export interface Props extends Table.PropsEx<Data, Context> {
		context: Context;
	}
}

export const UserTable: FC<UserTable.Props> = (props) => {
	const invalidator = useInvalidator([["GameMap"], ["Management"], ["User_Inventory"]]);

	const resetGameMutation = useMutation({
		async mutationFn({ userId }: { userId: string }) {
			return transaction(async (tx) => {
				await tx.deleteFrom("Map").where("userId", "=", userId).execute();
				// await tx
				// 	.deleteFrom("Production")
				// 	.where("userId", "=", userId)
				// 	.execute();
				// await tx
				// 	.deleteFrom("Construction")
				// 	.where("userId", "=", userId)
				// 	.execute();
			});
		},
		async onSuccess() {
			await invalidator();
		},
	});

	return (
		<Table
			columns={columns}
			actionRow={({ data }) => {
				return (
					<ActionMenu>
						<ActionClick
							icon={TrashIcon}
							onClick={() => {
								resetGameMutation.mutateAsync({ userId: data.id });
							}}
						>
							<Tx label={"Reset game (menu)"} />
						</ActionClick>
					</ActionMenu>
				);
			}}
			{...props}
		/>
	);
};
