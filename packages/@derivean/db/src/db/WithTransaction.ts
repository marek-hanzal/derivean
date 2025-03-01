/** @format */

import type { Database } from "@derivean/sdk";
import type { Transaction } from "kysely";

export type WithTransaction = Transaction<Database>;
