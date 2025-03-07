/** @format */

import { transaction } from "@derivean/db";
import { serviceFillInventory } from "@derivean/service";
import { InventoryIcon, InventoryTypeInline } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import {
	ActionClick,
	ActionMenu,
	ActionModal,
	DeleteControl,
	Progress,
	Table,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withEqualFilter,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, toHumanNumber, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { InventoryForm } from "../inventory/Form";
import type { ResourceTable } from "../resource/ResourceTable";

export namespace BlueprintInventoryTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		amount: number;
		limit: number;
		resourceId: string;
		inventoryId: string;
		type: "storage" | "construction";
	}
}

const column = withColumn<BlueprintInventoryTable.Data>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Resource name (label)"} />;
		},
		render({ value }) {
			return value;
		},
		filter: withEqualFilter({ path: "resourceId" }),
		size: 18,
	}),
	column({
		name: "type",
		header() {
			return <Tx label={"Inventory type (label)"} />;
		},
		render({ value }) {
			return <InventoryTypeInline label={value} />;
		},
		filter: withEqualFilter({ path: "type" }),
		size: 12,
	}),
	column({
		name: "amount",
		header() {
			return <Tx label={"Amount (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 12,
	}),
	column({
		name: "limit",
		header() {
			return <Tx label={"Inventory limit (label)"} />;
		},
		render({ value, data }) {
			return (
				<div className={"flex flex-row items-center gap-2 w-full"}>
					<div>{toHumanNumber({ number: value })}</div>
					<Progress value={(100 * data.amount) / data.limit} />
				</div>
			);
		},
		size: 14,
	}),
];

export namespace BlueprintInventoryTable {
	export interface Props extends Table.PropsEx<Data> {
		resourceTableContext: ResourceTable.Context;
		blueprintId: string;
	}
}

export const BlueprintInventoryTable: FC<BlueprintInventoryTable.Props> = ({
	resourceTableContext,
	blueprintId,
	...props
}) => {
	const invalidator = useInvalidator([["Blueprint_Inventory"]]);
	const fillInventoryMutation = useMutation({
		async mutationFn() {
			return transaction(async (tx) => {
				return serviceFillInventory({ tx, blueprintId });
			});
		},
		async onSuccess() {
			await invalidator({});
		},
		onError(error) {
			console.error(error);
		},
	});

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create inventory item (menu)"} />}
								textTitle={<Tx label={"Create inventory item (modal)"} />}
								icon={InventoryIcon}
							>
								<InventoryForm
									resourceTableContext={resourceTableContext}
									mutation={useMutation({
										async mutationFn(values) {
											return toast.promise(
												transaction(async (tx) => {
													return tx
														.insertInto("Blueprint_Inventory")
														.values({
															id: genId(),
															blueprintId,
															inventoryId: (
																await tx
																	.insertInto("Inventory")
																	.values({
																		id: genId(),
																		...values,
																	})
																	.returningAll()
																	.executeTakeFirstOrThrow()
															).id,
														})
														.returningAll()
														.executeTakeFirstOrThrow();
												}),
												withToastPromiseTx("Create inventory item"),
											);
										},
										async onSuccess() {
											await invalidator();
										},
									})}
								/>
							</ActionModal>
							<ActionClick
								icon={InventoryIcon}
								onClick={() => {
									fillInventoryMutation.mutate();
								}}
							>
								<Tx label={"Fill inventory (label)"} />
							</ActionClick>
						</ActionMenu>
					);
				},
				row({ data }) {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Edit (menu)"} />}
								textTitle={<Tx label={"Edit inventory item (modal)"} />}
								icon={InventoryIcon}
							>
								<InventoryForm
									resourceTableContext={resourceTableContext}
									defaultValues={data}
									mutation={useMutation({
										async mutationFn(values) {
											return toast.promise(
												transaction(async (tx) => {
													return tx
														.updateTable("Inventory")
														.set(values)
														.where("id", "=", data.inventoryId)
														.returningAll()
														.executeTakeFirstOrThrow();
												}),
												withToastPromiseTx("Update inventory item"),
											);
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
								textTitle={<Tx label={"Delete inventory item (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										return transaction(async (tx) => {
											return tx
												.deleteFrom("Inventory")
												.where("id", "=", data.inventoryId)
												.execute();
										});
									}}
									textContent={<Tx label={"Inventory item delete (content)"} />}
									textToast={"Inventory item delete"}
									invalidator={invalidator}
								/>
							</ActionModal>
						</ActionMenu>
					);
				},
			}}
			{...props}
		/>
	);
};
