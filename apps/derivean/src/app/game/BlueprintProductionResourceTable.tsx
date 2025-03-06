/** @format */

import { Table, Tx, withColumn, withEqualFilter } from "@use-pico/client";
import { toHumanNumber, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";

export namespace BlueprintProductionResourceTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		resourceId: string;
		amount: number;
	}
}

const column = withColumn<BlueprintProductionResourceTable.Data>();

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

export namespace BlueprintProductionResourceTable {
	export interface Props extends Table.PropsEx<Data> {
		//
	}
}

export const BlueprintProductionResourceTable: FC<BlueprintProductionResourceTable.Props> = (
	props,
) => {
	return (
		<Table
			columns={columns}
			{...props}
		/>
	);
};
