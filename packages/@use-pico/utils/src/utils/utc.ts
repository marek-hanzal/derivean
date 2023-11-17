import {DateTime} from "luxon";

export const utc = () => DateTime.utc().toISO()!;
