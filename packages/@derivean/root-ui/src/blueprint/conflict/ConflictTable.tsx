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
	Table,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, type IdentitySchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";
import type { BlueprintTable } from "../Table";
import { ConflictForm } from "./ConflictForm";

export namespace ConflictTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		blueprintId: string;
		conflictId: string;
	}

	export interface Context {
		linkConflict: FC<{
			icon: string;
			to: "/$locale/root/blueprint/$id/conflicts";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
	}
}

const column = withColumn<ConflictTable.Data, ConflictTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Conflict building (label)"} />;
		},
		render({ data, value, context: { linkConflict: LinkConflict } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
				<LinkConflict
					icon={BlueprintIcon}
					to={"/$locale/root/blueprint/$id/conflicts"}
					params={{ locale, id: data.conflictId }}
				>
					{value}
				</LinkConflict>
			);
		},
		size: 22,
	}),
];

export namespace ConflictTable {
	export interface Props extends Table.PropsEx<Data, Context> {
		blueprintTableContext: BlueprintTable.Context;
		blueprintId: string;
		context: Context;
	}
}

export const ConflictTable: FC<ConflictTable.Props> = ({
	blueprintTableContext,
	blueprintId,
	...props
}) => {
	const invalidator = useInvalidator([["Blueprint_Conflict"], ["Blueprint"]]);

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create blueprint conflict (menu)"} />}
								textTitle={<Tx label={"Create blueprint conflict (modal)"} />}
								icon={BlueprintIcon}
							>
								{({ close }) => {
									return (
										<ConflictForm
											blueprintTableContext={blueprintTableContext}
											mutation={useMutation({
												async mutationFn(values) {
													return toast.promise(
														kysely.transaction().execute(async (tx) => {
															const entity = tx
																.insertInto("Blueprint_Conflict")
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
															"Create blueprint conflict",
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
								textTitle={<Tx label={"Edit blueprint conflict (modal)"} />}
								icon={BlueprintIcon}
							>
								{({ close }) => {
									return (
										<ConflictForm
											blueprintTableContext={blueprintTableContext}
											defaultValues={data}
											mutation={useMutation({
												async mutationFn(values) {
													return toast.promise(
														kysely.transaction().execute(async (tx) => {
															return tx
																.updateTable("Blueprint_Conflict")
																.set(values)
																.where("id", "=", data.id)
																.returningAll()
																.executeTakeFirstOrThrow();
														}),
														withToastPromiseTx(
															"Update blueprint conflict",
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
								textTitle={<Tx label={"Delete blueprint conflict (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										return kysely.transaction().execute(async (tx) => {
											return tx
												.deleteFrom("Blueprint_Conflict")
												.where("id", "=", data.id)
												.execute();
										});
									}}
									textContent={
										<Tx label={"Delete blueprint conflict (content)"} />
									}
									textToast={"Delete blueprint conflict"}
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

export { ConflictTable as BlueprintConflictTable };
