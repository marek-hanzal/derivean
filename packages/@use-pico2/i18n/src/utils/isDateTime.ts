import {isObject}      from "@use-pico2/utils";
import {type DateTime} from "luxon";

export const isDateTime = (input: any): input is DateTime => {
    return isObject(input) && ("toJSDate" in input);
};
