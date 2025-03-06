/** @format */

import { useRouteContext } from "@tanstack/react-router";

export const useRootTva = () => {
	return useRouteContext({ from: "__root__" }).tva;
};
