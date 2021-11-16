#include "playback_buffer.h"

playback_buffer::playback_buffer()
{
    //ctor
    ts0=0;
    ts_lp=0;
    length = 0; //current length of playback buffer
	max_size = MAX_PBBFR_SIZE; //maximum length of playback buffer
	max_time = MAX_PBBFR_TIME;
    last_packet = first_packet = NULL;
	isReadyToPlay = false;
}

playback_buffer::~playback_buffer()
{
    //dtor
    show_pn();
    clear_pbb();
}

//copy packet is used in the pgm
int playback_buffer::copy_packet (pbb_packet *from, pbb_packet *to)
{
    //copy all fields from one pbb_packet to another
	to->next = from->next;
	int i;
	for (i = 0; i < from->pl_size; i++)
		to->data[i] = from->data[i];
	to->nr = from->nr;
	to->nt = from->nt;
	to->p_t.frame = from->p_t.frame;
	to->p_t.pred_num = from->p_t.pred_num;
	to->gop_num = from->gop_num;
	to->pl_size = from->pl_size;
	to->p2p_pn = from->p2p_pn;
	to->pn = from->pn;
    to->packet_id = from->packet_id;
	return EXIT_SUCCESS;
}

//Play is used in the pgm
bool playback_buffer::play(int s, struct sockaddr_in addr, unsigned long &pn, unsigned char &nr, std::string &id)
{
    if (!isReadyToPlay)
    {
        if ((ts_lp-ts0) > max_time)
        {
            isReadyToPlay = true;
        }
        return false;
    }
    else
    {
        if (first_packet!=NULL)
        {
            pn = first_packet->pn; //first packet number for statistics
            nr = first_packet->nr; //first packet repeat numer for statistics
            id = first_packet->packet_id;
            //printf("pn=%lu",pn);
            //printf("id=%s",first_packet->packet_id.c_str());
        }
        else
            printf("[NULL PBB]");
                send_first_packet (s,addr);
    }
    return true;
}
bool playback_buffer::shift_buffer()
{
    //shift a buffer if it's time
    if (length >= max_size)
	{
        add_packet(&new_packet);
        delete_first_packet();
        return true;
    }
	else
	{
    	add_packet(&new_packet);
    	return false;
	}
}

bool playback_buffer::shift_buffer (int s, struct sockaddr_in addr, unsigned long &pn)
{
    //shift a buffer and send first packet if it's time
	if (length >= max_size)
	{
	    pn = first_packet->pn; //first packet number for statistics
		send_first_packet (s, addr);
        add_packet(&new_packet);
        return true;
    }
	else
	{
        add_packet(&new_packet);
    	return false;
	}
}

bool playback_buffer::check_buffer (int s, struct sockaddr_in addr, unsigned long &pn)
{
    //shift a buffer and send first packet if it's time
	if (length > max_size)
	{
	    pn = first_packet->pn; //first packet number for statistics
		send_first_packet (s, addr);
        add_packet(&new_packet);
    }
    else
        return false;
    return true;
}

//delete_first_packet is used in the pgm
int playback_buffer::delete_first_packet()
{
	pbb_packet *temp;
	if (first_packet == NULL)
	{
		return EXIT_SUCCESS;
	}
	else if (first_packet->next == NULL)
	{
		delete (first_packet);
		isReadyToPlay=false;
		ts0=0;
		ts_lp=0;
		length=0;
		return EXIT_SUCCESS;
	}
	else
	{
	    temp = first_packet->next;
	    delete (first_packet);
	    first_packet = NULL;
	    first_packet = temp;
		length--;
		return EXIT_SUCCESS;
	}
    return EXIT_FAILURE;
}

//send_first_packet is used in the pgm
int playback_buffer::send_first_packet (int s, struct sockaddr_in addr)
{

  if (first_packet->packet_id == "192.168.1.20")
    {
      addr.sin_port = htons(5020);
      sendto(s, (char *) first_packet->data, first_packet->pl_size - ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof (struct sockaddr));
    }
  else if (first_packet->packet_id == "192.168.1.6")
    {
      addr.sin_port = htons(5030);
      sendto(s, (char *) first_packet->data, first_packet->pl_size - ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof (struct sockaddr));
    }
  else if (first_packet->packet_id == "192.168.1.3")
    {
      addr.sin_port = htons(5040);
      sendto(s, (char *) first_packet->data, first_packet->pl_size - ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof (struct sockaddr));
    }


    delete_first_packet();
    return EXIT_SUCCESS;
/*
        sendto(s, (char *) first_packet->data, first_packet->pl_size - ARQ_HDR_SIZE, 0, (struct sockaddr *) &addr, sizeof (struct sockaddr));
    delete_first_packet();
    return EXIT_SUCCESS;

*/
}

//add_packet is used in the pgm
int playback_buffer::add_packet (pbb_packet *packet)
{
	pbb_packet 	*temp = new pbb_packet,
				*cur,
		   		*prev;

	if (!length) // if length = O
	{
		copy_packet (&new_packet, temp);
		first_packet = temp;
		last_packet = temp;
		ts0 = GetTickCount();

	}
	else if ((first_packet->pn > packet->pn)&&(first_packet->packet_id == packet->packet_id))
	{
		copy_packet (first_packet, temp);
		copy_packet (&new_packet, first_packet);
		first_packet->next = temp;
	}
	else if (first_packet->next == NULL)
	{
		copy_packet (&new_packet, temp);
		first_packet->next = temp;
		//last_packet = temp;
	}
//	else if (last_packet->pn < packet->pn)
//	{
//		copy_packet (&new_packet, temp);
//		last_packet->next = temp;
//		last_packet = temp;
//	}
    else
    {
        copy_packet (&new_packet, temp);
        prev = first_packet;
        cur = first_packet->next;
        while (cur != NULL)
        {
            if(cur->packet_id != packet->packet_id)
            {
                prev = cur;
                cur = prev->next;
            }
            else if((cur->pn < packet->pn) && (cur->packet_id == packet->packet_id))
            {
                prev = cur;
                cur = prev->next;
            }
            else
                break;

        }
        if ((cur!=NULL)&&(cur->pn == packet->pn)&& (cur->packet_id == packet->packet_id))
        {
            return EXIT_FAILURE;

        }
        prev->next = temp;
        temp->next = cur;
    }
/*	else
	{
		copy_packet (&new_packet, temp);
		prev = first_packet;
		cur = first_packet->next;
		while ((cur != NULL) && (cur->pn < packet->pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if ((cur!=NULL)&&(cur->pn == packet->pn))
		{
		    return EXIT_FAILURE;
		}
		prev->next = temp;
		temp->next = cur;

	}
*/

	length++;
	ts_lp = GetTickCount();
	return EXIT_SUCCESS;
}

int playback_buffer::get_packet_by_p2p_pn (unsigned long p2p_pn, unsigned char nt)
{
	pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if ((first_packet->nt == nt) && (first_packet->p2p_pn == p2p_pn))
    {
        copy_packet (first_packet, &new_packet);
        return EXIT_SUCCESS;
    }
	else
	{
		prev = first_packet;
		cur = first_packet->next;
        while (cur != NULL)
		{
		    if ((cur->nt == nt) && (cur->p2p_pn == p2p_pn))
                break;
            prev = cur;
            cur = prev->next;
		}
		if (cur != NULL)
		{
			copy_packet (cur, &new_packet);
			return EXIT_SUCCESS;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
}

int playback_buffer::get_pn_by_p2p_pn (unsigned long p2p_pn, unsigned char nt, unsigned long &pn)
{
    pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if ((first_packet->nt == nt) && (first_packet->p2p_pn == p2p_pn))
    {
        pn = first_packet->pn;
        return EXIT_SUCCESS;
    }
	else
	{
		prev = first_packet;
		cur = first_packet->next;
        while (cur != NULL)
		{
		    if ((cur->nt == nt) && (cur->p2p_pn == p2p_pn))
                break;
            prev = cur;
            cur = prev->next;
		}
		if (cur != NULL)
		{
		    pn = cur->pn;
			return EXIT_SUCCESS;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
}

int playback_buffer::get_packet_by_pn (unsigned long pn)
{
	pbb_packet	*cur,
		   		*prev;
	if (length == 0 || first_packet == NULL)
	{
		return EXIT_FAILURE;
	}
	else if (first_packet->pn > pn)
	{
		return EXIT_FAILURE;
	}
	else if (first_packet->pn == pn)
	{
		copy_packet (first_packet, &new_packet);
		return EXIT_SUCCESS;
	}
	else if ((new_packet.next != NULL) && (new_packet.next->pn == pn))
	{
		copy_packet (new_packet.next, &new_packet);
		return EXIT_SUCCESS;
	}
	else if (last_packet->pn < pn)
	{
		return EXIT_FAILURE;
	}
	else if ((new_packet.next != NULL) && (new_packet.next->pn < pn))
	{
		prev = new_packet.next;
		cur = new_packet.next->next;
		while ((cur != NULL) && (cur->pn < pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if ((cur != NULL) && (cur->pn == pn))
		{
			copy_packet (cur, &new_packet);
		}
		else
		{
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	else
	{
		prev = first_packet;
		cur = first_packet->next;
		while ((cur != NULL) && (cur->pn < pn))
		{
			prev = cur;
			cur = prev->next;
		}
		if ((cur != NULL) && (cur->pn == pn))
		{
			copy_packet (cur, &new_packet);
		}
		else
		{
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
}

// clear_pbb is used in the pgm
int playback_buffer::clear_pbb ()
{
	while (length != 0)
		delete_first_packet();
	return EXIT_SUCCESS;
}

//show_pn is used in the pgm
int playback_buffer::show_pn ()
{
	pbb_packet *cur = first_packet;
	int i;
	printf ("\nPBB%d: ", length);
	for (i = 0; i < length; i++)
	{
		if (cur != NULL)
		{
			printf ("%lu;", cur->pn);
			cur = cur->next;
		}
	}
	printf ("\n");
	return EXIT_SUCCESS;
}
