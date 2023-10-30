import {type DateTime} from "@use-pico2/i18n";
import {StoreProvider} from "@use-pico2/store";
import {
    type FC,
    type PropsWithChildren
}                      from "react";
import {weeksOf}       from "../calendar/weeksOf";
import {WeeksOfStore}  from "./WeeksOfStore";

export namespace WeeksOfProvider {
    export type  Props = PropsWithChildren<{
        date?: DateTime;
        selected?: DateTime;
    }>
}

export const WeeksOfProvider: FC<WeeksOfProvider.Props> = (
    {
        date,
        selected,
        ...props
    }) => {
    return <StoreProvider
        store={WeeksOfStore}
        values={{
            weeks: weeksOf({
                date,
                selected,
            }),
        }}
        {...props}
    />;
};
