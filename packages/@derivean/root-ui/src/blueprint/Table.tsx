/** @format */

import { transaction } from "@derivean/db";
import {
	serviceBlueprintSort,
	serviceFillInventory,
	type serviceBlueprintGraph,
} from "@derivean/service";
import { BlueprintIcon, CyclesInline, InventoryIcon } from "@derivean/ui";
import { RequirementsInline } from "@derivean/ui/src/ui/RequirementsInline";
import {
	toWebp64,
	type BlueprintDependencySchema,
	type BlueprintRequirementSchema,
} from "@derivean/utils";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionClick,
	ActionMenu,
	ActionModal,
	DeleteControl,
	Table,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, toHumanNumber, tvc, type IdentitySchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";
import { BlueprintDependenciesInline } from "./DependenciesInline";
import { BlueprintForm } from "./Form";

export namespace BlueprintTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		cycles: number;
		sort: number;
		limit: number;
		requirements: (BlueprintRequirementSchema["~entity"] & { name: string })[];
		dependencies: (BlueprintDependencySchema["~entity"] & { name: string })[];
		graph?: string;
	}

	export interface Context {
		dependencies?: serviceBlueprintGraph.Result;
		linkView: FC<{
			icon: string | ReactNode;
			to: "/$locale/root/blueprint/$id/view";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
		linkEditor: FC<{
			icon: string | ReactNode;
			to: "/$locale/root/editor";
			params: { locale: string };
			search: { zoomTo: string };
		}>;
	}
}

const column = withColumn<BlueprintTable.Data, BlueprintTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Building name (label)"} />;
		},
		render({ data, value, context: { linkView: LinkView, linkEditor: LinkEditor } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<div className={"flex flex-row gap-2 items-center"}>
					<LinkView
						icon={
							<div
								className={tvc([
									"border-2",
									"border-purple-400",
									"rounded-md",
									"w-[64px]",
									"h-[64px]",
									"bg-contain",
									`bg-${data.id}`,
								])}
							/>
						}
						to={"/$locale/root/blueprint/$id/view"}
						params={{ locale, id: data.id }}
					>
						{value}
					</LinkView>

					<LinkEditor
						icon={"icon-[ph--graph-light]"}
						to={"/$locale/root/editor"}
						params={{ locale }}
						search={{ zoomTo: data.id }}
					/>
				</div>
			);
		},
		size: 18,
	}),
	column({
		name: "dependencies",
		header() {
			return <Tx label={"Blueprint dependencies (label)"} />;
		},
		render({ value }) {
			return value.length > 0 ? (
				<div className={"flex flex-col flex-wrap gap-2 w-full"}>
					<BlueprintDependenciesInline
						textTitle={<Tx label={"Blueprint dependencies (title)"} />}
						dependencies={value}
					/>
					<div className={"border-b border-slate-200"} />
				</div>
			) : (
				<Tx label={"No dependencies (label)"} />
			);
		},
		size: 32,
	}),
	column({
		name: "requirements",
		header() {
			return <Tx label={"Required resources (label)"} />;
		},
		render({ value }) {
			return (
				<RequirementsInline
					textTitle={<Tx label={"Blueprint requirements (title)"} />}
					textEmpty={<Tx label={"No requirements (label)"} />}
					requirements={value}
				/>
			);
		},
		size: 32,
	}),
	column({
		name: "cycles",
		header() {
			return <Tx label={"Construction cycles (label)"} />;
		},
		render({ value }) {
			return <CyclesInline cycles={value} />;
		},
		size: 8,
	}),
	column({
		name: "limit",
		header() {
			return <Tx label={"Building limit (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 8,
	}),
];

export namespace BlueprintTable {
	export interface Props extends Table.PropsEx<Data, BlueprintTable.Context> {
		context: Context;
	}
}

export const BlueprintTable: FC<BlueprintTable.Props> = (props) => {
	const invalidator = useInvalidator([["Blueprint_Inventory"], ["Blueprint"], ["Inventory"]]);

	const fillInventoryMutation = useMutation({
		async mutationFn() {
			return transaction(async (tx) => {
				const blueprints = await tx.selectFrom("Blueprint").select(["id"]).execute();

				for await (const { id: blueprintId } of blueprints) {
					await serviceFillInventory({ tx, blueprintId });
				}
			});
		},
		async onSuccess() {
			await invalidator();
		},
		onError(error) {
			console.error(error);
		},
	});

	return (
		<Table
			columns={columns}
			actionTable={() => {
				return (
					<ActionMenu>
						<ActionClick
							icon={InventoryIcon}
							onClick={() => {
								toast.promise(
									fillInventoryMutation.mutateAsync(),
									withToastPromiseTx("Fill inventories"),
								);
							}}
						>
							<Tx label={"Fill inventories (label)"} />
						</ActionClick>
						<ActionModal
							label={<Tx label={"Create blueprint (menu)"} />}
							textTitle={<Tx label={"Create blueprint (modal)"} />}
							icon={BlueprintIcon}
						>
							{({ close }) => {
								return (
									<BlueprintForm
										mutation={useMutation({
											async mutationFn({ image, ...values }) {
												transaction(async (tx) => {
													const entity = await tx
														.insertInto("Blueprint")
														.values({
															id: genId(),
															...values,
															image: image
																? await toWebp64(image)
																: null,
														})
														.returningAll()
														.executeTakeFirstOrThrow();

													await serviceBlueprintSort({ tx });

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
							textTitle={<Tx label={"Edit blueprint (modal)"} />}
							icon={BlueprintIcon}
						>
							{({ close }) => {
								return (
									<BlueprintForm
										defaultValues={data}
										mutation={useMutation({
											async mutationFn({ image, ...values }) {
												return transaction(async (tx) => {
													const entity = await tx
														.updateTable("Blueprint")
														.set({
															...values,
															image: image
																? await toWebp64(image)
																: null,
														})
														.where("id", "=", data.id)
														.returningAll()
														.executeTakeFirstOrThrow();

													await serviceBlueprintSort({ tx });

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

						<ActionModal
							icon={TrashIcon}
							label={<Tx label={"Delete (menu)"} />}
							textTitle={<Tx label={"Delete blueprint (modal)"} />}
							css={{
								base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
							}}
						>
							<DeleteControl
								callback={async () => {
									return transaction(async (tx) => {
										return tx
											.deleteFrom("Blueprint")
											.where("id", "=", data.id)
											.execute();
									});
								}}
								textContent={<Tx label={"Delete blueprint (content)"} />}
								textToast={"Delete blueprint"}
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
