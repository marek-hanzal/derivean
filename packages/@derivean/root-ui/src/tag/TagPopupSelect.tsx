/** @format */

import { transaction } from "@derivean/db";
import { TagSchema } from "@derivean/utils";
import { PopupSelect, withListCount } from "@use-pico/client";
import type { FC } from "react";
import { TagTable } from "./TagTable";

export namespace TagPopupSelect {
	export interface Props extends PopupSelect.PropsEx<TagTable.Data> {
		group?: string;
	}
}

export const TagPopupSelect: FC<TagPopupSelect.Props> = ({ group, ...props }) => {
	return (
		<PopupSelect<TagTable.Data>
			table={(props) => (
				<TagTable
					group={group}
					{...props}
				/>
			)}
			render={({ entity }) => {
				return entity.label;
			}}
			queryKey={"Tag"}
			query={async ({ filter, cursor }) => {
				return transaction(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Tag as t")
							.select(["t.id", "t.code", "t.label", "t.group", "t.sort"]),
						output: TagSchema.entity,
						query({ select, where }) {
							let $select = select;

							if (where?.id) {
								$select = $select.where("t.id", "=", where.id);
							}

							if (where?.fulltext) {
								const fulltext = `%${where.fulltext}%`.toLowerCase();

								$select = $select.where((qb) => {
									return qb.or([
										qb("t.code", "like", fulltext),
										qb("t.group", "like", fulltext),
										qb("t.label", "like", fulltext),
										qb("t.id", "like", fulltext),
									]);
								});
							}

							return $select;
						},
						filter,
						cursor,
					});
				});
			}}
			{...props}
		/>
	);
};
