import type { Transaction } from "kysely";
import type { Database } from "~/app/db/sdk";

export type WithTransaction = Transaction<Database>;
