import { createFileRoute, useLoaderData } from "@tanstack/react-router";

export const Route = createFileRoute("/$locale/root/region/$id/view")({
	component() {
		const {
			entity: { image },
		} = useLoaderData({
			from: "/$locale/root/region/$id",
		});

		return <>{image ? <img src={image || undefined} /> : "no image"}</>;
	},
});
