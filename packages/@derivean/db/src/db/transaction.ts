/** @format */

import { kysely } from "./kysely";
import type { WithTransaction } from "./WithTransaction";

export const transaction = <T>(callback: (trx: WithTransaction) => Promise<T>) =>
	kysely.transaction().execute(callback);
