/** @format */

import { transaction } from "@derivean/db";
import { BlueprintIcon } from "@derivean/ui";
import { BlueprintDependencySchema, BlueprintRequirementSchema } from "@derivean/utils";
import { PopupSelect as CoolPopupSelect, Tx, withListCount } from "@use-pico/client";
import { withJsonOutputArraySchema } from "@use-pico/common";
import { sql } from "kysely";
import type { FC } from "react";
import { z } from "zod";
import { BlueprintTable } from "./Table";

namespace PopupSelect {
	export interface Props extends CoolPopupSelect.PropsEx<BlueprintTable.Data> {
		context: BlueprintTable.Context;
	}
}

const PopupSelect: FC<PopupSelect.Props> = ({ context, ...props }) => {
	return (
		<CoolPopupSelect<BlueprintTable.Data>
			icon={BlueprintIcon}
			textTitle={<Tx label={"Select blueprint (title)"} />}
			table={(props) => {
				return (
					<BlueprintTable
						context={context}
						hidden={["requirements", "dependencies"]}
						{...props}
					/>
				);
			}}
			render={({ entity }) => {
				return entity.name;
			}}
			queryKey={"Blueprint"}
			query={async ({ filter, cursor }) => {
				return transaction(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Blueprint as b")
							.select([
								"b.id",
								"b.name",
								"b.sort",
								"b.cycles",
								"b.limit",
								(eb) =>
									eb
										.selectFrom("Blueprint_Requirement as br")
										.innerJoin("Resource as r", "r.id", "br.resourceId")
										.select((eb) => {
											return sql<string>`json_group_array(json_object(
                                                    'id', ${eb.ref("br.id")},
                                                    'amount', ${eb.ref("br.amount")},
                                                    'passive', ${eb.ref("br.passive")},
                                                    'resourceId', ${eb.ref("br.resourceId")},
                                                    'blueprintId', ${eb.ref("br.blueprintId")},
                                                    'name', ${eb.ref("r.name")}
                                                ))`.as("requirements");
										})
										.whereRef("br.blueprintId", "=", "b.id")
										.as("requirements"),
								(eb) =>
									eb
										.selectFrom("Blueprint_Dependency as bd")
										.innerJoin("Blueprint as b", "b.id", "bd.blueprintId")
										.select((eb) => {
											return sql<string>`json_group_array(json_object(
                                                    'id', ${eb.ref("bd.id")},
                                                    'dependencyId', ${eb.ref("bd.dependencyId")},
                                                    'blueprintId', ${eb.ref("bd.blueprintId")},
                                                    'name', ${eb.ref("b.name")}
                                                ))`.as("requirements");
										})
										.whereRef("bd.blueprintId", "=", "b.id")
										.as("dependencies"),
							])
							.orderBy("b.name", "asc"),
						query({ select, where }) {
							let $select = select;

							if (where?.id) {
								$select = $select.where("b.id", "=", where.id);
							}

							if (where?.fulltext) {
								const fulltext = `%${where.fulltext}%`.toLowerCase();

								$select = $select.where((eb) => {
									return eb.or([
										eb("b.id", "like", fulltext),
										eb("b.name", "like", fulltext),
									]);
								});
							}

							return $select;
						},
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							cycles: z.number().nonnegative(),
							sort: z.number().nonnegative(),
							limit: z.number().nonnegative(),
							requirements: withJsonOutputArraySchema(
								BlueprintRequirementSchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
							dependencies: withJsonOutputArraySchema(
								BlueprintDependencySchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
						}),
						filter,
						cursor,
					});
				});
			}}
			{...props}
		/>
	);
};

export { PopupSelect as BlueprintPopupSelect };
