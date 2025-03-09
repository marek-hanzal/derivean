/** @format */

import { transaction } from "@derivean/db";
import { BlueprintIcon, BuildingIcon } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	Table,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
} from "@use-pico/client";
import { type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import type { BlueprintTable } from "../blueprint/Table";
import { BuildingForm } from "./Form";

export namespace BuildingTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		blueprintId: string;
	}

	export interface Context {
		linkView: FC<{
			icon: string;
			to: "/$locale/root/blueprint/$id/view";
			params: { locale: string; id: string };
		}>;
	}
}

const column = withColumn<BuildingTable.Data, BuildingTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Building name (label)"} />;
		},
		render({ data, value, context: { linkView: LinkView } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<div className={"flex flex-row gap-2"}>
					<LinkView
						icon={BlueprintIcon}
						to={"/$locale/root/blueprint/$id/view"}
						params={{ locale, id: data.blueprintId }}
					/>

					{value}
				</div>
			);
		},
		size: 14,
	}),
];

export namespace BuildingTable {
	export interface Props extends Table.PropsEx<Data, Context> {
		context: Context;
		blueprintTableContext: BlueprintTable.Context;
	}
}

export const BuildingTable: FC<BuildingTable.Props> = ({ blueprintTableContext, ...props }) => {
	const invalidator = useInvalidator([["Building"]]);

	return (
		<Table
			columns={columns}
			actionRow={({ data }) => {
				return (
					<ActionMenu>
						<ActionModal
							label={<Tx label={"Edit (menu)"} />}
							textTitle={<Tx label={"Edit building (modal)"} />}
							icon={BuildingIcon}
						>
							<BuildingForm
								blueprintTableContext={blueprintTableContext}
								defaultValues={data}
								mutation={useMutation({
									async mutationFn(values) {
										return transaction((tx) => {
											return tx
												.updateTable("Building")
												.set(values)
												.where("id", "=", data.id)
												.returningAll()
												.executeTakeFirstOrThrow();
										});
									},
									async onSuccess() {
										await invalidator();
									},
								})}
							/>
						</ActionModal>

						<ActionModal
							icon={TrashIcon}
							label={<Tx label={"Delete (menu)"} />}
							textTitle={<Tx label={"Delete building (modal)"} />}
							css={{
								base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
							}}
						>
							<DeleteControl
								callback={async () => {
									return transaction(async (tx) => {
										return tx
											.deleteFrom("Building")
											.where("id", "=", data.id)
											.execute();
									});
								}}
								textContent={<Tx label={"Building delete (content)"} />}
								textToast={"Building delete"}
								invalidator={invalidator}
							/>
						</ActionModal>
					</ActionMenu>
				);
			}}
			{...props}
		/>
	);
};
