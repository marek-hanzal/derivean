import {type DateTime} from "@use-pico2/i18n";
import {StoreProvider} from "@use-pico2/store";
import {
    type FC,
    type PropsWithChildren
}                      from "react";
import {monthsOf}      from "../calendar/monthsOf";
import {MonthsOfStore} from "./MonthsOfStore";

export namespace MonthsOfProvider {
    export type Props = PropsWithChildren<{
        date?: DateTime;
        selected?: DateTime;
    }>
}

export const MonthsOfProvider: FC<MonthsOfProvider.Props> = (
    {
        date,
        selected,
        ...props
    }) => {
    return <StoreProvider
        store={MonthsOfStore}
        values={{
            months: monthsOf({
                date,
                selected,
            }),
        }}
        {...props}
    />;
};
