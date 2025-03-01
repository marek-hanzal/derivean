/** @format */

import { Progress, Table, Tx, useTable, withColumn, withEqualFilter } from "@use-pico/client";
import { toHumanNumber, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace InventoryTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		amount: number;
		limit: number;
		resourceId: string;
		storage?: number;
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
		name: "amount",
		header() {
			return <Tx label={"Amount (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 18,
	}),
	column({
		name: "limit",
		header() {
			return <Tx label={"Inventory limit (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 14,
	}),
	column({
		name: "storage",
		header() {
			return <Tx label={"Storage (label)"} />;
		},
		render({ data }) {
			return <Progress value={(100 * data.amount) / data.limit} />;
		},
		size: 24,
	}),
];

export namespace InventoryTable {
	export interface Props extends Table.PropsEx<Data> {
		//
	}
}

export const InventoryTable: FC<InventoryTable.Props> = ({ table, ...props }) => {
	return (
		<Table
			table={useTable({ ...table, columns })}
			{...props}
		/>
	);
};
