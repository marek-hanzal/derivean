/** @format */

import { kysely } from "@derivean/db";
import { ResourceIcon } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	Table,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withEqualFilter,
} from "@use-pico/client";
import { genId, toHumanNumber, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { ResourceTable as CoolResourceTable } from "../../../resource/Table";
import { ResourceForm } from "./ResourceForm";

export namespace ResourceTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		resourceId: string;
		amount: number;
	}
}

const column = withColumn<ResourceTable.Data>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Requirement name (label)"} />;
		},
		render({ value }) {
			return value;
		},
		filter: withEqualFilter({ path: "resourceId" }),
		size: 22,
	}),
	column({
		name: "amount",
		header() {
			return <Tx label={"Amount (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 10,
	}),
];

export namespace ResourceTable {
	export interface Props extends Table.PropsEx<Data> {
		resourceTableContext: CoolResourceTable.Context;
		blueprintProductionId: string;
	}
}

export const ResourceTable: FC<ResourceTable.Props> = ({
	resourceTableContext,
	blueprintProductionId,
	...props
}) => {
	const invalidator = useInvalidator([
		["Blueprint_Production_Resource"],
		["Blueprint_Production"],
		["Resource"],
	]);

	return (
		<Table
			columns={columns}
			actionTable={() => {
				return (
					<ActionMenu>
						<ActionModal
							label={<Tx label={"Create production resource requirement (menu)"} />}
							textTitle={
								<Tx label={"Create production resource requirement (modal)"} />
							}
							icon={ResourceIcon}
						>
							{({ close }) => {
								return (
									<ResourceForm
										resourceTableContext={resourceTableContext}
										mutation={useMutation({
											async mutationFn(values) {
												return kysely.transaction().execute(async (tx) => {
													return tx
														.insertInto("Blueprint_Production_Resource")
														.values({
															id: genId(),
															...values,
															blueprintProductionId,
														})
														.returningAll()
														.executeTakeFirstOrThrow();
												});
											},
											async onSuccess() {
												await invalidator();
												close();
											},
										})}
									/>
								);
							}}
						</ActionModal>
					</ActionMenu>
				);
			}}
			actionRow={({ data }) => {
				return (
					<ActionMenu>
						<ActionModal
							label={<Tx label={"Edit (menu)"} />}
							textTitle={
								<Tx label={"Edit production resource requirement (modal)"} />
							}
							icon={ResourceIcon}
						>
							{({ close }) => {
								return (
									<ResourceForm
										resourceTableContext={resourceTableContext}
										defaultValues={data}
										mutation={useMutation({
											async mutationFn(values) {
												return kysely.transaction().execute(async (tx) => {
													return tx
														.updateTable(
															"Blueprint_Production_Resource",
														)
														.set(values)
														.where("id", "=", data.id)
														.returningAll()
														.executeTakeFirstOrThrow();
												});
											},
											async onSuccess() {
												await invalidator();
												close();
											},
										})}
									/>
								);
							}}
						</ActionModal>

						<ActionModal
							icon={TrashIcon}
							label={<Tx label={"Delete (menu)"} />}
							textTitle={
								<Tx label={"Delete production resource requirement (modal)"} />
							}
							css={{
								base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
							}}
						>
							<DeleteControl
								callback={async () => {
									return kysely.transaction().execute(async (tx) => {
										return tx
											.deleteFrom("Blueprint_Production_Resource")
											.where("id", "=", data.id)
											.execute();
									});
								}}
								textContent={
									<Tx
										label={"Delete production resource requirement (content)"}
									/>
								}
								textToast={"Delete production resource requirement item"}
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

export { ResourceTable as BlueprintProductionResourceTable };
