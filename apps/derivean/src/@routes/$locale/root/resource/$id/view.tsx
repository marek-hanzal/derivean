import { createFileRoute, useLoaderData } from "@tanstack/react-router";

export const Route = createFileRoute("/$locale/root/resource/$id/view")({
	component() {
		const {
			entity: { image },
		} = useLoaderData({
			from: "/$locale/root/resource/$id",
		});

		return (
			<>
				<img src={image || undefined} />
			</>
		);
	},
});
