/** @format */

import { transaction } from "@derivean/db";
import type { LoginSchema, SessionSchema } from "@derivean/utils";
import { useMutation } from "@tanstack/react-query";
import { pwd } from "@use-pico/common";

export const useLoginMutation = () => {
	return useMutation({
		mutationKey: ["useLoginMutation"],
		async mutationFn({ login, password }: LoginSchema.Type): Promise<SessionSchema.Type> {
			return transaction(async (tx) => {
				const user = await tx
					.selectFrom("User as u")
					.select(["u.id", "u.login", "u.name"])
					.where("u.login", "=", login)
					.where("u.password", "=", pwd.hash(password))
					.executeTakeFirstOrThrow();

				return { id: user.id, login: user.login, name: user.name };
			});
		},
	});
};
