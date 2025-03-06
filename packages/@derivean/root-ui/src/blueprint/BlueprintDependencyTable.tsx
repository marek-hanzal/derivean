/** @format */

import { kysely } from "@derivean/db";
import { serviceBlueprintSort } from "@derivean/service";
import { BlueprintIcon } from "@derivean/ui";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	LinkTo,
	Table,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { BlueprintDependencyForm } from "./BlueprintDependencyForm";
import type { BlueprintTable } from "./BlueprintTable";

export namespace BlueprintDependencyTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		blueprintId: string;
		dependencyId: string;
	}
}

const column = withColumn<BlueprintDependencyTable.Data>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Required building (label)"} />;
		},
		render({ data, value }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<LinkTo
					icon={BlueprintIcon}
					to={"/$locale/root/blueprint/$id/view"}
					params={{ locale, id: data.dependencyId }}
				>
					{value}
				</LinkTo>
			);
		},
		size: 22,
	}),
];

export namespace BlueprintDependencyTable {
	export interface Props extends Table.PropsEx<Data> {
		blueprintTableContext: BlueprintTable.Context;
		blueprintId: string;
	}
}

export const BlueprintDependencyTable: FC<BlueprintDependencyTable.Props> = ({
	blueprintTableContext,
	blueprintId,
	...props
}) => {
	const invalidator = useInvalidator([["Blueprint_Dependency"], ["Blueprint"]]);

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create blueprint dependency (menu)"} />}
								textTitle={<Tx label={"Create blueprint dependency (modal)"} />}
								icon={BlueprintIcon}
							>
								{({ close }) => {
									return (
										<BlueprintDependencyForm
											blueprintTableContext={blueprintTableContext}
											mutation={useMutation({
												async mutationFn(values) {
													return toast.promise(
														kysely.transaction().execute(async (tx) => {
															const entity = tx
																.insertInto("Blueprint_Dependency")
																.values({
																	id: genId(),
																	...values,
																	blueprintId,
																})
																.returningAll()
																.executeTakeFirstOrThrow();

															await serviceBlueprintSort({ tx });

															return entity;
														}),
														withToastPromiseTx(
															"Create blueprint dependency",
														),
													);
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
								textTitle={<Tx label={"Edit blueprint dependency (modal)"} />}
								icon={BlueprintIcon}
							>
								{({ close }) => {
									return (
										<BlueprintDependencyForm
											blueprintTableContext={blueprintTableContext}
											defaultValues={data}
											mutation={useMutation({
												async mutationFn(values) {
													return toast.promise(
														kysely.transaction().execute(async (tx) => {
															return tx
																.updateTable("Blueprint_Dependency")
																.set(values)
																.where("id", "=", data.id)
																.returningAll()
																.executeTakeFirstOrThrow();
														}),
														withToastPromiseTx(
															"Update blueprint dependency",
														),
													);
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
								textTitle={<Tx label={"Delete blueprint dependency (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										return kysely.transaction().execute(async (tx) => {
											return tx
												.deleteFrom("Blueprint_Dependency")
												.where("id", "=", data.id)
												.execute();
										});
									}}
									textContent={
										<Tx label={"Delete blueprint dependency (content)"} />
									}
									textToast={"Delete blueprint dependency"}
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
