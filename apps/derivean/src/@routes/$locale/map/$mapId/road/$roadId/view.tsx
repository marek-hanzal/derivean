import { createFileRoute } from "@tanstack/react-router";

export const Route = createFileRoute(
	"/$locale/map/$mapId/road/$roadId/view",
)({
	component() {
		return "mrdka";
	},
});
