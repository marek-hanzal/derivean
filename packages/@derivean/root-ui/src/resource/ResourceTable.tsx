/** @format */

import { transaction } from "@derivean/db";
import { ResourceIcon } from "@derivean/ui";
import { toWebp64 } from "@derivean/utils";
import { useMutation } from "@tanstack/react-query";
import { useParams } from "@tanstack/react-router";
import {
	ActionMenu,
	ActionModal,
	Badge,
	DeleteControl,
	Table,
	Tags,
	toast,
	TrashIcon,
	Tx,
	useInvalidator,
	withColumn,
	withToastPromiseTx,
} from "@use-pico/client";
import { genId, toHumanNumber, tvc, type IdentitySchema, type TagSchema } from "@use-pico/common";
import type { FC, ReactNode } from "react";
import { ResourceForm } from "./ResourceForm";

export namespace ResourceTable {
	export interface Data extends IdentitySchema.Type {
		name: string;
		weight: number;
		tags: TagSchema.Type[];
		countRequirement: number;
		countProduction: number;
		countProductionRequirement: number;
	}

	export interface Context {
		linkView: FC<{
			icon: ReactNode;
			to: "/$locale/root/resource/$id/view";
			params: { locale: string; id: string };
			children: ReactNode;
		}>;
	}
}

const column = withColumn<ResourceTable.Data, ResourceTable.Context>();

const columns = [
	column({
		name: "name",
		header() {
			return <Tx label={"Resource name (label)"} />;
		},
		render({ data, value, context: { linkView: LinkView } }) {
			const { locale } = useParams({ from: "/$locale" });

			return (
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
					to={"/$locale/root/resource/$id/view"}
					params={{ locale, id: data.id }}
				>
					{value}
				</LinkView>
			);
		},
		size: 18,
	}),
	column({
		name: "weight",
		header() {
			return <Tx label={"Resource weight (label)"} />;
		},
		render({ value }) {
			return toHumanNumber({ number: value });
		},
		size: 12,
	}),
	column({
		name: "countProduction",
		header() {
			return <Tx label={"Resource production count (label)"} />;
		},
		render({ value }) {
			return (
				<Badge
					css={{
						base:
							value !== undefined && value === 0
								? ["bg-red-100", "text-red-500", "border-red-600"]
								: [],
					}}
				>
					{toHumanNumber({ number: value })}
				</Badge>
			);
		},
		size: 12,
	}),
	column({
		name: "countRequirement",
		header() {
			return <Tx label={"Resource requirement count (label)"} />;
		},
		render({ data, value }) {
			return (
				<Badge
					css={{
						base:
							data.countRequirement === 0 && data.countProductionRequirement === 0
								? ["bg-red-100", "text-red-500", "border-red-600"]
								: [],
					}}
				>
					{toHumanNumber({ number: value })}
				</Badge>
			);
		},
		size: 12,
	}),
	column({
		name: "countProductionRequirement",
		header() {
			return <Tx label={"Resource production requirement count (label)"} />;
		},
		render({ data, value }) {
			return (
				<Badge
					css={{
						base:
							data.countRequirement === 0 && data.countProductionRequirement === 0
								? ["bg-red-100", "text-red-500", "border-red-600"]
								: [],
					}}
				>
					{toHumanNumber({ number: value })}
				</Badge>
			);
		},
		size: 18,
	}),
	column({
		name: "tags",
		header() {
			return <Tx label={"Resource tags (label)"} />;
		},
		render({ value }) {
			return <Tags tags={value} />;
		},
		size: 32,
	}),
];

export namespace ResourceTable {
	export interface Props extends Table.PropsEx<Data, Context> {
		context: Context;
		group?: string;
	}
}

export const ResourceTable: FC<ResourceTable.Props> = ({ group, ...props }) => {
	const invalidator = useInvalidator([["Resource"], ["Resource_Tag"]]);

	return (
		<Table
			columns={columns}
			action={{
				table() {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Create resource (menu)"} />}
								textTitle={<Tx label={"Create resource (modal)"} />}
								icon={ResourceIcon}
							>
								{({ close }) => {
									return (
										<ResourceForm
											group={group}
											mutation={useMutation({
												async mutationFn({
													image,
													tagIds = [],
													...values
												}) {
													return transaction(async (tx) => {
														const entity = await tx
															.insertInto("Resource")
															.values({
																id: genId(),
																...values,
																image: image
																	? await toWebp64(image)
																	: null,
															})
															.returningAll()
															.executeTakeFirstOrThrow();

														if (tagIds.length) {
															await tx
																.insertInto("Resource_Tag")
																.values(
																	tagIds.map((tagId) => ({
																		id: genId(),
																		resourceId: entity.id,
																		tagId,
																	})),
																)
																.execute();
														}

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
				},
				row({ data }) {
					return (
						<ActionMenu>
							<ActionModal
								label={<Tx label={"Edit (menu)"} />}
								textTitle={<Tx label={"Edit resource (modal)"} />}
								icon={ResourceIcon}
							>
								{({ close }) => {
									return (
										<ResourceForm
											defaultValues={{
												...data,
												tagIds: data.tags.map(({ id }) => id),
											}}
											mutation={useMutation({
												async mutationFn({ image, tagIds, ...rest }) {
													return toast.promise(
														transaction(async (tx) => {
															const entity = await tx
																.updateTable("Resource")
																.set({
																	...rest,
																	image: image
																		? await toWebp64(image)
																		: null,
																})
																.where("id", "=", data.id)
																.returningAll()
																.executeTakeFirstOrThrow();

															await tx
																.deleteFrom("Resource_Tag")
																.where("resourceId", "=", entity.id)
																.execute();

															if (tagIds?.length) {
																await tx
																	.insertInto("Resource_Tag")
																	.values(
																		tagIds.map((tagId) => ({
																			id: genId(),
																			resourceId: entity.id,
																			tagId,
																		})),
																	)
																	.execute();
															}

															return entity;
														}),
														withToastPromiseTx("Update resource"),
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
								textTitle={<Tx label={"Delete resource (modal)"} />}
								css={{
									base: ["text-red-500", "hover:text-red-600", "hover:bg-red-50"],
								}}
							>
								<DeleteControl
									callback={async () => {
										return transaction(async (tx) => {
											return tx
												.deleteFrom("Resource")
												.where("id", "=", data.id)
												.execute();
										});
									}}
									textContent={<Tx label={"Resource delete (content)"} />}
									textToast={"Resource delete"}
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
