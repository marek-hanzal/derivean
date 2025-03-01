/** @format */

import { transaction, type WithTransaction } from "@derivean/db";
import { useMutation } from "@tanstack/react-query";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	Progress,
	Table,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	useTable,
	withColumn,
	withEqualFilter,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, toHumanNumber, type Entity, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { InventoryTypeInline } from "~/app/inventory/InventoryTypeInline";
import { InventoryForm } from "~/app/root/InventoryForm";
import type { InventorySchema } from "~/app/schema/InventorySchema";
import { InventoryIcon } from "../../../../../packages/@derivean/ui/src/icon/InventoryIcon";

export namespace InventoryTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		amount: number;
		limit: number;
		resourceId: string;
		type: "storage" | "construction";
	}
}

const column = withColumn<InventoryTable.Data>();

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

export namespace InventoryTable {
	export namespace onCreate {
		export interface Props extends Entity.Schema<InventorySchema["entity"]> {
			tx: WithTransaction;
		}
	}

	export interface Props extends Table.PropsEx<Data> {
		onCreate?(props: onCreate.Props): Promise<any>;
	}
}

export const InventoryTable: FC<InventoryTable.Props> = ({ onCreate, table, ...props }) => {
	const invalidator = useInvalidator([
		["Inventory"],
		["User_Inventory"],
		["Building_Base_Production"],
	]);

	return (
		<Table
			table={useTable({ ...table, columns })}
			action={{
				table: onCreate
					? () => {
							return (
								<ActionMenu>
									<ActionModal
										label={<Tx label={"Create inventory item (menu)"} />}
										textTitle={<Tx label={"Create inventory item (modal)"} />}
										icon={InventoryIcon}
									>
										<InventoryForm
											mutation={useMutation({
												async mutationFn(values) {
													return toast.promise(
														transaction(async (tx) => {
															const entity = await tx
																.insertInto("Inventory")
																.values({ id: genId(), ...values })
																.returningAll()
																.executeTakeFirstOrThrow();

															await onCreate?.({ tx, entity });

															return entity;
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
								</ActionMenu>
							);
						}
					: undefined,
				row({ data }) {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Edit (menu)"} />}
								textTitle={<Tx label={"Edit inventory item (modal)"} />}
								icon={InventoryIcon}
							>
								<InventoryForm
									defaultValues={data}
									mutation={useMutation({
										async mutationFn(values) {
											return toast.promise(
												transaction(async (tx) => {
													return tx
														.updateTable("Inventory")
														.set(values)
														.where("id", "=", data.id)
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
												.where("id", "=", data.id)
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
