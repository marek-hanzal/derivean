import {type DateTime} from "luxon";

export const isDateTime = (input: any): input is DateTime => {
    return input && "toJSDate" in input;
};
