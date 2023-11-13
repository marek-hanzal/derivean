import {
    KingdomMenu,
    withKingdomRepository
}                    from "@derivean/kingdom";
import {container}   from "@derivean/server";
import {KingdomIcon} from "@derivean/ui";
import {t}           from "@use-pico/i18n";
import {Page}        from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Index({params: {kingdomId}}: Index.Props) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <Page
        icon={<KingdomIcon/>}
        text={{
            header: t({values: kingdom})`Kingdom overview`,
        }}
        append={<KingdomMenu
            kingdomId={kingdomId}
        />}
    >
        buildings, ...
    </Page>;
}
