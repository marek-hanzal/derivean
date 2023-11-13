import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {EventIcon}             from "@derivean/ui";
import {t}                     from "@use-pico/i18n";
import {Page}                  from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Events({params: {kingdomId}}: Index.Props) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <Page
        icon={<EventIcon/>}
        text={{
            header: t()`Events (label)`,
        }}
    >
        Event listing here
    </Page>;
}
