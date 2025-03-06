/** @format */

import { transaction } from "@derivean/db";
import { BlueprintForm } from "@derivean/root-ui";
import { useMutation } from "@tanstack/react-query";
import { createFileRoute, useLoaderData } from "@tanstack/react-router";
import { useInvalidator } from "@use-pico/client";
import { withBase64 } from "@use-pico/common";

export const Route = createFileRoute("/$locale/root/blueprint/$id/edit")({
	component: () => {
		const { entity } = useLoaderData({ from: "/$locale/root/blueprint/$id" });
		const navigate = Route.useNavigate();
		const invalidator = useInvalidator([["Blueprint"]]);

		return (
			<div className={"w-1/2 mx-auto"}>
				<BlueprintForm
					defaultValues={entity}
					mutation={useMutation({
						async mutationFn({ image, ...values }) {
							return transaction(async (tx) => {
								await tx
									.updateTable("Blueprint")
									.set({
										...values,
										image: image ? await withBase64(image) : null,
									})
									.where("id", "=", entity.id)
									.returningAll()
									.executeTakeFirstOrThrow();
							});
						},
						async onSuccess() {
							await invalidator();
							navigate({
								to: "/$locale/root/blueprint/$id/view",
								params: { id: entity.id },
							});
						},
					})}
				/>
			</div>
		);
	},
});
