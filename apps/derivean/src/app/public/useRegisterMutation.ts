/** @format */

import { transaction } from "@derivean/db";
import { useMutation } from "@tanstack/react-query";
import { genId, pwd } from "@use-pico/common";
import type { RegisterSchema } from "~/app/schema/RegisterSchema";
import { SessionSchema } from "~/app/schema/SessionSchema";

export const useRegisterMutation = () => {
	return useMutation({
		mutationKey: ["useRegisterMutation"],
		async mutationFn({ login, name, password1 }: RegisterSchema.Type): Promise<SessionSchema.Type> {
			return transaction(async (tx) => {
				/**
				 * Secondary SessionSchema parse is here to ensure only session related data
				 * get out.
				 */
				const session = SessionSchema.parse(
					await tx
						.insertInto("User")
						.values({ id: genId(), name, login, password: pwd.hash(password1) })
						.returning(["User.id", "User.name", "User.login"])
						.executeTakeFirstOrThrow(),
				);

				return session;
			});
		},
	});
};
