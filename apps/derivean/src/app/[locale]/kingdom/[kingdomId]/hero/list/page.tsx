import {
    HeroQueryProvider,
    HeroTable
}                              from "@derivean/hero";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {HeroIcon}              from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function List({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<HeroIcon/>}
        text={{
            header: t`Kingdom hero list (label)`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href: "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
            ]}
        />}
    >
        <HeroQueryProvider
            values={{
                where: {
                    kingdomId: kingdom.id,
                },
            }}
        >
            <HeroTable
                kingdomId={kingdom.id}
            />
        </HeroQueryProvider>
    </Page>;
}
