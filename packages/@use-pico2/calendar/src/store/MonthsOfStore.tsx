import {DateTime}     from "@use-pico2/i18n";
import {
    createStore,
    type IStore
}                     from "@use-pico2/store";
import {type IMonths} from "../api/months";
import {monthsOf}     from "../calendar/monthsOf";

export namespace MonthsOfStore {
    export type StoreProps = IStore<{
        /**
         * Set months of the given date
         */
        monthsOf(date: DateTime): IMonths;
        /**
         * Move to the current month
         */
        today(): IMonths;
        prevYear(): IMonths;
        nextYear(): IMonths;
    }, {
        /**
         * Calendar is computed based on an input, so it cannot be required
         * in the time of store creation.
         */
        readonly months: IMonths;
    }>
}

export const MonthsOfStore = createStore<MonthsOfStore.StoreProps>(values => (set, get) => ({
    monthsOf(date: DateTime) {
        set(({months: {selected}}) => ({
            months: monthsOf({
                date,
                selected,
            }),
        }));
        return get().months;
    },
    today() {
        set(({months: {selected}}) => ({
            months: monthsOf({
                date: DateTime.now(),
                selected,
            }),
        }));
        return get().months;
    },
    prevYear() {
        set(({
                 months: {
                             date,
                             selected
                         }
             }) => ({
            months: monthsOf({
                date: date.minus({year: 1}),
                selected,
            }),
        }));
        return get().months;
    },
    nextYear() {
        set(({
                 months: {
                             date,
                             selected
                         }
             }) => ({
            months: monthsOf({
                date: date.plus({year: 1}),
                selected,
            }),
        }));
        return get().months;
    },
    ...values,
}));
