/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { useParams } from "@tanstack/react-router";
import { LinkTo, Table, Tx, useTable, withColumn } from "@use-pico/client";
import { type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

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
					to={"/$locale/game/blueprint/$id/view"}
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
		//
	}
}

export const BlueprintDependencyTable: FC<BlueprintDependencyTable.Props> = ({ table, ...props }) => {
	return (
		<Table
			table={useTable({ ...table, columns })}
			{...props}
		/>
	);
};
