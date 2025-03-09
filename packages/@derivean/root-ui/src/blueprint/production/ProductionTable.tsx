/** @format */

import { transaction } from "@derivean/db";
import { ProductionIcon, ResourceIcon } from "@derivean/ui";
import { RequirementsInline } from "@derivean/ui/src/ui/RequirementsInline";
import type {
	BlueprintProductionDependencySchema,
	BlueprintProductionRequirementSchema,
	BlueprintProductionResourceSchema,
} from "@derivean/utils";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	More,
	Table,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
} from "@use-pico/client";
import { genId, toHumanNumber, tvc, type IdentitySchema } from "@use-pico/common";
import { type FC, type ReactNode } from "react";
import type { ResourceTable as CoolResourceTable } from "../../resource/Table";
import { MoveProductionToForm } from "../MoveProductionToForm";
import type { BlueprintTable } from "../Table";
import { BlueprintProductionForm } from "./ProductionForm";

export namespace ProductionTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		resourceId: string;
		amount: number;
		cycles: number;
		requirements: (BlueprintProductionRequirementSchema["~entity"] & { name: string })[];
		resources: (BlueprintProductionResourceSchema["~entity"] & { name: string })[];
		dependencies: (BlueprintProductionDependencySchema["~entity"] & { name: string })[];
	}

	export interface Context {
		linkRequirements: FC<{
			icon: string;
			to: "/$locale/root/blueprint/production/$id/requirements";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
	}
}

const column = withColumn<ProductionTable.Data, ProductionTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Resource name (label)"} />;
		},
		render({ data, value, context: { linkRequirements: LinkRequirements } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<LinkRequirements
					icon={ProductionIcon}
					to={"/$locale/root/blueprint/production/$id/requirements"}
					params={{ locale, id: data.id }}
				>
					{value}
				</LinkRequirements>
			);
		},
		size: 18,
	}),
	column({
		name: "amount",
		header() {
			return <Tx label={"Amount (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 8,
	}),
	column({
		name: "cycles",
		header() {
			return <Tx label={"Production cycles (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 8,
	}),
	column({
		name: "requirements",
		header() {
			return <Tx label={"Required resources (label)"} />;
		},
		render({ value }) {
			return (
				<RequirementsInline
					textTitle={<Tx label={"Resource requirements (title)"} />}
					textEmpty={<Tx label={"No requirements (label)"} />}
					requirements={value}
				/>
			);
		},
		size: 32,
	}),
	column({
		name: "resources",
		header() {
			return <Tx label={"Required production resources (label)"} />;
		},
		render({ value }) {
			return (
				<More<ProductionTable.Data["resources"][number]>
					items={value}
					render={({ entity }) => {
						return (
							<div
								className={tvc([
									"flex",
									"flex-row",
									"gap-2",
									"items-center",
									"bg-sky-100",
									"border",
									"rounded-sm",
									"border-sky-300",
									"py-1",
									"px-2",
								])}
							>
								<div>{entity.name}</div>
								<div className={"text-md font-bold text-slate-500"}>
									x{toHumanNumber({ number: entity.amount })}
								</div>
							</div>
						);
					}}
				/>
			);
		},
		size: 32,
	}),
	column({
		name: "dependencies",
		header() {
			return <Tx label={"Required production dependencies (label)"} />;
		},
		render({ value }) {
			return (
				<More<ProductionTable.Data["dependencies"][number]>
					items={value}
					render={({ entity }) => {
						return (
							<div
								className={tvc([
									"flex",
									"flex-row",
									"gap-2",
									"items-center",
									"bg-sky-100",
									"border",
									"rounded-sm",
									"border-sky-300",
									"py-1",
									"px-2",
								])}
							>
								<div>{entity.name}</div>
							</div>
						);
					}}
				/>
			);
		},
		size: 32,
	}),
];

export namespace ProductionTable {
	export interface Props extends Table.PropsEx<Data, Context> {
		context: Context;
		blueprintTableContext: BlueprintTable.Context;
		resourceTableContext: CoolResourceTable.Context;
		blueprintId: string;
	}
}

export const ProductionTable: FC<ProductionTable.Props> = ({
	blueprintTableContext,
	resourceTableContext,
	blueprintId,
	...props
}) => {
	const invalidator = useInvalidator([
		["Blueprint"],
		["Blueprint_Production"],
		["Blueprint_Production_Requirement"],
		["Resource"],
	]);

	return (
		<Table
			columns={columns}
			actionTable={() => {
				return (
					<ActionMenu>
						<ActionModal
							label={<Tx label={"Create blueprint production (menu)"} />}
							textTitle={<Tx label={"Create blueprint production (modal)"} />}
							icon={ProductionIcon}
						>
							{({ close }) => {
								return (
									<BlueprintProductionForm
										resourceTableContext={resourceTableContext}
										mutation={useMutation({
											async mutationFn(values) {
												return transaction(async (tx) => {
													const entity = await tx
														.insertInto("Blueprint_Production")
														.values({
															id: genId(),
															...values,
															blueprintId,
														})
														.returningAll()
														.executeTakeFirstOrThrow();

													return entity;
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
							textTitle={<Tx label={"Edit blueprint production (modal)"} />}
							icon={ProductionIcon}
						>
							<BlueprintProductionForm
								resourceTableContext={resourceTableContext}
								defaultValues={data}
								mutation={useMutation({
									async mutationFn(values) {
										return transaction(async (tx) => {
											return tx
												.updateTable("Blueprint_Production")
												.set(values)
												.where("id", "=", data.id)
												.returningAll()
												.executeTakeFirstOrThrow();
										});
									},
									async onSuccess() {
										await invalidator({});
									},
								})}
							/>
						</ActionModal>

						<ActionModal
							label={<Tx label={"Move production to (menu)"} />}
							textTitle={<Tx label={"Move production to (modal)"} />}
							icon={ResourceIcon}
						>
							{({ close }) => {
								return (
									<MoveProductionToForm
										blueprintTableContext={blueprintTableContext}
										mutation={useMutation({
											async mutationFn(values) {
												return transaction(async (tx) => {
													await tx
														.updateTable("Blueprint_Production")
														.set({ blueprintId: values.blueprintId })
														.where("id", "=", data.id)
														.execute();
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
							textTitle={<Tx label={"Delete blueprint production (modal)"} />}
							css={{
								base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
							}}
						>
							<DeleteControl
								callback={async () => {
									return transaction(async (tx) => {
										return tx
											.deleteFrom("Blueprint_Production")
											.where("id", "=", data.id)
											.execute();
									});
								}}
								textContent={<Tx label={"Delete blueprint production (content)"} />}
								textToast={"Delete blueprint production"}
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

export { ProductionTable as BlueprintProductionTable };
