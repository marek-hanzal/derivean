import { createFileRoute } from "@tanstack/react-router";

export const Route = createFileRoute(
	"/$locale/root/user/$id/view",
)({
	component() {
		return "yep";
	},
});
