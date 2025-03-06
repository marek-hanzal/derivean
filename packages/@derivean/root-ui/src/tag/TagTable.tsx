/** @format */

import { transaction } from "@derivean/db";
import { useMutation } from "@tanstack/react-query";
import {
	ActionMenu,
	ActionModal,
	DeleteControl,
	Table,
	TagIcon,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withEqualFilter,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, type IdentitySchema } from "@use-pico/common";
import type { FC } from "react";
import { TagForm } from "./TagForm";

export namespace TagTable {
	export interface Data extends IdentitySchema.Type {
		code: string;
		label: string;
		group?: string | null;
		sort: number;
	}
}

const column = withColumn<TagTable.Data>();

const columns = [
	column({
		name: "label",
		header() {
			return <Tx label={"Tag label (label)"} />;
		},
		render({ value }) {
			return value;
		},
		size: 14,
	}),
	column({
		name: "code",
		header() {
			return <Tx label={"Tag code (label)"} />;
		},
		render({ value }) {
			return value;
		},
		size: 14,
	}),
	column({
		name: "group",
		header() {
			return <Tx label={"Tag group (label)"} />;
		},
		render({ value }) {
			return value;
		},
		filter: withEqualFilter({ path: "group" }),
		size: 24,
	}),
];

export namespace TagTable {
	export interface Props extends Table.PropsEx<Data> {
		group?: string;
	}
}

export const TagTable: FC<TagTable.Props> = ({ group, ...props }) => {
	const invalidator = useInvalidator([["Tag"], ["Resource"]]);

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create tag (menu)"} />}
								textTitle={<Tx label={"Create tag (modal)"} />}
								icon={TagIcon}
							>
								<TagForm
									defaultValues={{ group }}
									mutation={useMutation({
										async mutationFn(values) {
											return toast.promise(
												transaction((tx) => {
													return tx
														.insertInto("Tag")
														.values({ id: genId(), ...values })
														.returningAll()
														.executeTakeFirstOrThrow();
												}),
												withToastPromiseTx("Create tag"),
											);
										},
										async onSuccess() {
											await invalidator();
										},
									})}
								/>
							</ActionModal>
						</ActionMenu>
					);
				},
				row({ data }) {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Edit (menu)"} />}
								textTitle={<Tx label={"Edit tag (modal)"} />}
								icon={TagIcon}
							>
								<TagForm
									defaultValues={{ group, ...data }}
									mutation={useMutation({
										async mutationFn(values) {
											return toast.promise(
												transaction((tx) => {
													return tx
														.updateTable("Tag")
														.set(values)
														.where("id", "=", data.id)
														.returningAll()
														.executeTakeFirstOrThrow();
												}),
												withToastPromiseTx("Edit tag"),
											);
										},
										async onSuccess() {
											await invalidator();
										},
									})}
								/>
							</ActionModal>

							<ActionModal
								icon={TrashIcon}
								label={<Tx label={"Delete (menu)"} />}
								textTitle={<Tx label={"Delete tag (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										await transaction((tx) => {
											return tx
												.deleteFrom("Tag")
												.where("id", "=", data.id)
												.execute();
										});
									}}
									textContent={<Tx label={"Tag delete (content)"} />}
									textToast={"Tag delete"}
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
