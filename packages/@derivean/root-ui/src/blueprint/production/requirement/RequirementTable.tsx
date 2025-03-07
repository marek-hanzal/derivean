/** @format */

import { kysely } from "@derivean/db";
import { ResourceIcon } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import {
	ActionMenu,
	ActionModal,
	BoolInline,
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
import type { ResourceTable } from "../../../resource/Table";
import { RequirementForm } from "./RequirementForm";

export namespace RequirementTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		resourceId: string;
		amount: number;
		passive: boolean;
	}
}

const column = withColumn<RequirementTable.Data>();

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
		size: 10,
	}),
	column({
		name: "passive",
		header() {
			return <Tx label={"Passive requirement (label)"} />;
		},
		render({ value }) {
			return <BoolInline value={value} />;
		},
		size: 10,
	}),
];

export namespace RequirementTable {
	export interface Props extends Table.PropsEx<Data> {
		resourceTableContext: ResourceTable.Context;
		blueprintProductionId: string;
	}
}

export const RequirementTable: FC<RequirementTable.Props> = ({
	resourceTableContext,
	blueprintProductionId,
	...props
}) => {
	const invalidator = useInvalidator([
		["Blueprint_Production_Requirement"],
		["Blueprint_Production"],
		["Resource"],
	]);

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create production requirement (menu)"} />}
								textTitle={<Tx label={"Create production requirement (modal)"} />}
								icon={ResourceIcon}
							>
								{({ close }) => {
									return (
										<RequirementForm
											resourceTableContext={resourceTableContext}
											mutation={useMutation({
												async mutationFn(values) {
													return kysely
														.transaction()
														.execute(async (tx) => {
															return tx
																.insertInto(
																	"Blueprint_Production_Requirement",
																)
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
				},
				row({ data }) {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Edit (menu)"} />}
								textTitle={<Tx label={"Edit production requirement (modal)"} />}
								icon={ResourceIcon}
							>
								{({ close }) => {
									return (
										<RequirementForm
											resourceTableContext={resourceTableContext}
											defaultValues={data}
											mutation={useMutation({
												async mutationFn(values) {
													return kysely
														.transaction()
														.execute(async (tx) => {
															return tx
																.updateTable(
																	"Blueprint_Production_Requirement",
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
								textTitle={<Tx label={"Delete production requirement (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										return kysely.transaction().execute(async (tx) => {
											return tx
												.deleteFrom("Blueprint_Production_Requirement")
												.where("id", "=", data.id)
												.execute();
										});
									}}
									textContent={
										<Tx label={"Delete production requirement (content)"} />
									}
									textToast={"Delete production requirement item"}
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

export { RequirementTable as BlueprintProductionRequirementTable };
